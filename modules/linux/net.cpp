// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

static_assert(sizeof(ssize_t) <= sizeof(int64_t));
using namespace std;
using namespace std::chrono;

struct event_data_t
{
    int fd;
    const size_t capacity;
    unique_ptr<epoll_event[]> events;

  public:
    event_data_t() noexcept(false)
        : fd{-1},
          // use 2 page for polling
          capacity{2 * getpagesize() / sizeof(epoll_event)},
          events{make_unique<epoll_event[]>(capacity)}
    {
        fd = epoll_create1(EPOLL_CLOEXEC);
        if (fd < 0)
            throw system_error{errno, system_category(), "epoll_create1"};
    }
    ~event_data_t() noexcept
    {
        close(fd);
    }

    void try_add(uint64_t sd, epoll_event& req) noexcept(false)
    {
        int op = EPOLL_CTL_ADD, ec = 0;
    TRY_OP:
        ec = epoll_ctl(fd, op, sd, &req);
        if (ec == 0)
            return;
        if (errno == EEXIST)
        {
            op = EPOLL_CTL_MOD; // already exists. try with modification
            goto TRY_OP;
        }

        throw system_error{errno, system_category(), "epoll_ctl"};
    }

    void remove(uint64_t sd)
    {
        epoll_event req{}; // just prevent non-null input
        const auto ec = epoll_ctl(fd, EPOLL_CTL_DEL, sd, &req);
        assert(ec == 0);
    }

    auto wait(int timeout) noexcept(false) -> enumerable<coroutine_task_t>
    {
        auto count = epoll_wait(fd, events.get(), capacity, timeout);
        if (count == -1)
            throw system_error{errno, system_category(), "epoll_wait"};

        for (auto i = 0; i < count; ++i)
        {
            auto& ev = events[i];
            auto task = coroutine_task_t::from_address(ev.data.ptr);
            co_yield task;
        }
    }
};

event_data_t inbound{}, outbound{};

auto wait_io_tasks(nanoseconds timeout) noexcept(false)
    -> enumerable<coroutine_task_t>
{
    const int half_time = duration_cast<milliseconds>(timeout).count() / 2;

    for (auto coro : inbound.wait(half_time))
        co_yield coro;
    for (auto coro : outbound.wait(half_time))
        co_yield coro;
}

bool io_work_t::ready() const noexcept
{
    // non blocking operation is expected
    // going to suspend
    if (fcntl(sd, F_GETFL, 0) & O_NONBLOCK)
        return false;

    // not configured. return true
    // and bypass to the blocking I/O
    return true;
}

uint32_t io_work_t::error() const noexcept
{
    return errc;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto send_to(uint64_t sd, const sockaddr_in& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    work.sd = sd;
    work.to = addressof(remote);
    work.addrlen = sizeof(sockaddr_in);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

auto send_to(uint64_t sd, const sockaddr_in6& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    work.sd = sd;
    work.to6 = addressof(remote);
    work.addrlen = sizeof(sockaddr_in6);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

void io_send_to::suspend(coroutine_task_t rh) noexcept(false)
{
    errc = 0;

    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    outbound.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_send_to::resume() noexcept
{
    auto sz = sendto(sd, buffer.data(), buffer.size_bytes(), //
                     0, addr, addrlen);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto recv_from(uint64_t sd, sockaddr_in& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    work.sd = sd;
    work.from = addressof(remote);
    work.addrlen = sizeof(sockaddr_in);
    work.buffer = buffer;
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

auto recv_from(uint64_t sd, sockaddr_in6& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    work.sd = sd;
    work.from6 = addressof(remote);
    work.addrlen = sizeof(sockaddr_in6);
    work.buffer = buffer;
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

void io_recv_from::suspend(coroutine_task_t rh) noexcept(false)
{
    errc = 0;

    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    inbound.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_recv_from::resume() noexcept
{
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, addr, addressof(addrlen));
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto send_stream(uint64_t sd, buffer_view_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_send&
{
    static_assert(sizeof(socklen_t) == sizeof(uint32_t));

    work.sd = sd;
    work.addrlen = flag;
    work.buffer = buffer;
    return *reinterpret_cast<io_send*>(addressof(work));
}

void io_send::suspend(coroutine_task_t rh) noexcept(false)
{
    errc = 0;

    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    outbound.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_send::resume() noexcept
{
    const auto flag = addrlen;
    const auto sz = send(sd, buffer.data(), buffer.size_bytes(), flag);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto recv_stream(uint64_t sd, buffer_view_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_recv&
{
    static_assert(sizeof(socklen_t) == sizeof(uint32_t));

    work.sd = sd;
    work.addrlen = flag;
    work.buffer = buffer;
    return *reinterpret_cast<io_recv*>(addressof(work));
}

void io_recv::suspend(coroutine_task_t rh) noexcept(false)
{
    errc = 0;

    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    inbound.try_add(sd, req);
}

int64_t io_recv::resume() noexcept
{
    const auto flag = addrlen;
    const auto sz = recv(sd, buffer.data(), buffer.size_bytes(), flag);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

errc peer_name(uint64_t sd, sockaddr_in6& ep) noexcept
{
    socklen_t len = sizeof(sockaddr_in6);
    errc ec{};

    if (getpeername(static_cast<int>(sd), reinterpret_cast<sockaddr*>(&ep),
                    &len))
        ec = errc{errno};
    return ec;
}

errc sock_name(uint64_t sd, sockaddr_in6& ep) noexcept
{
    socklen_t len = sizeof(sockaddr_in6);
    errc ec{};

    if (getsockname(static_cast<int>(sd), reinterpret_cast<sockaddr*>(&ep),
                    &len))
        ec = errc{errno};
    return ec;
}
