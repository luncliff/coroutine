// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>

#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

static_assert(sizeof(ssize_t) <= sizeof(int64_t));
using namespace std;
using namespace std::chrono;

struct kqueue_data_t
{
    int fd;
    const size_t capacity;
    unique_ptr<kevent64_s[]> events;

  public:
    kqueue_data_t() noexcept(false)
        : fd{-1},
          // use 2 page for polling
          capacity{2 * getpagesize() / sizeof(kevent64_s)},
          events{make_unique<kevent64_s[]>(capacity)}
    {
        fd = kqueue();
        if (fd < 0)
            throw system_error{errno, system_category(), "kqueue"};
    }
    ~kqueue_data_t() noexcept
    {
        close(fd);
    }
};

kqueue_data_t kq{};

auto wait_io_tasks(nanoseconds timeout) noexcept(false)
    -> enumerable<coroutine_task_t>
{
    timespec ts{};
    const auto sec = duration_cast<seconds>(timeout);
    ts.tv_sec = sec.count();
    ts.tv_nsec = (timeout - sec).count();

    // wait for events ...
    auto count = kevent64(kq.fd, nullptr, 0,            //
                          kq.events.get(), kq.capacity, //
                          0, &ts);
    if (count == -1)
        throw system_error{errno, system_category(), "kevent64"};

    for (auto i = 0; i < count; ++i)
    {
        auto& ev = kq.events[i];
        auto& work = *reinterpret_cast<io_work_t*>(ev.udata);
        // need to pass error information from
        // kevent to io_work
        co_yield work.task;
    }
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
    static_assert(sizeof(void*) <= sizeof(uint64_t));
    task = rh;

    // one-shot, write registration (edge-trigger)
    kevent64_s req{};
    req.ident = sd;
    req.filter = EVFILT_WRITE;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    auto ec = kevent64(kq.fd, &req, 1, // change
                       nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw system_error{errno, system_category(), "kevent64"};
}

int64_t io_send_to::resume() noexcept
{
    auto sz = sendto(sd, buffer.data(), buffer.size_bytes(), //
                     0, addr, addrlen);
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
    static_assert(sizeof(void*) <= sizeof(uint64_t));

    task = rh;
    // system operation
    kevent64_s req{};
    req.ident = sd;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;

    // it is possible to pass `rh` for the user data,
    // but will pass this object to support
    // receiving some values from `wait_io_tasks`
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    // attach the event config
    auto ec = kevent64(kq.fd, &req, 1, nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw system_error{errno, system_category(), "kevent64"};
}

int64_t io_recv_from::resume() noexcept
{
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, addr, addressof(addrlen));
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
    static_assert(sizeof(void*) <= sizeof(uint64_t));
    task = rh;

    // one-shot, write registration (edge-trigger)
    kevent64_s req{};
    req.ident = sd;
    req.filter = EVFILT_WRITE;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    auto ec = kevent64(kq.fd, &req, 1, // change
                       nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw system_error{errno, system_category(), "kevent64"};
}

int64_t io_send::resume() noexcept
{
    const auto flag = addrlen;
    const auto sz = send(sd, buffer.data(), buffer.size_bytes(), flag);
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
    static_assert(sizeof(void*) <= sizeof(uint64_t));

    task = rh;
    // system operation
    kevent64_s req{};
    req.ident = sd;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    // attach the event config
    auto ec = kevent64(kq.fd, &req, 1, nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw system_error{errno, system_category(), "kevent64"};
}

int64_t io_recv::resume() noexcept
{
    const auto flag = addrlen;
    const auto sz = recv(sd, buffer.data(), buffer.size_bytes(), flag);
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

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

void print_kevent(const kevent64_s& ev)
{
    printf(" ev.ident \t: %llu\n", ev.ident);

    if (ev.filter == EVFILT_READ)
        printf(" ev.filter\t: EVFILT_READ\n");
    if (ev.filter == EVFILT_WRITE)
        printf(" ev.filter\t: EVFILT_WRITE\n");
    if (ev.filter == EVFILT_EXCEPT)
        printf(" ev.filter\t: EVFILT_EXCEPT\n");
    if (ev.filter == EVFILT_AIO)
        printf(" ev.filter\t: EVFILT_AIO\n");
    if (ev.filter == EVFILT_VNODE)
        printf(" ev.filter\t: EVFILT_VNODE\n");
    if (ev.filter == EVFILT_PROC)
        printf(" ev.filter\t: EVFILT_PROC\n");
    if (ev.filter == EVFILT_SIGNAL)
        printf(" ev.filter\t: EVFILT_SIGNAL\n");
    if (ev.filter == EVFILT_MACHPORT)
        printf(" ev.filter\t: EVFILT_MACHPORT\n");
    if (ev.filter == EVFILT_TIMER)
        printf(" ev.filter\t: EVFILT_TIMER\n");

    if (ev.flags & EV_ADD)
        printf(" ev.flags\t: EV_ADD\n");
    if (ev.flags & EV_DELETE)
        printf(" ev.flags\t: EV_DELETE\n");
    if (ev.flags & EV_ENABLE)
        printf(" ev.flags\t: EV_ENABLE\n");
    if (ev.flags & EV_DISABLE)
        printf(" ev.flags\t: EV_DISABLE\n");
    if (ev.flags & EV_ONESHOT)
        printf(" ev.flags\t: EV_ONESHOT\n");
    if (ev.flags & EV_CLEAR)
        printf(" ev.flags\t: EV_CLEAR\n");
    if (ev.flags & EV_RECEIPT)
        printf(" ev.flags\t: EV_RECEIPT\n");
    if (ev.flags & EV_DISPATCH)
        printf(" ev.flags\t: EV_DISPATCH\n");
    if (ev.flags & EV_UDATA_SPECIFIC)
        printf(" ev.flags\t: EV_UDATA_SPECIFIC\n");
    if (ev.flags & EV_VANISHED)
        printf(" ev.flags\t: EV_VANISHED\n");
    if (ev.flags & EV_ERROR)
        printf(" ev.flags\t: EV_ERROR\n");
    if (ev.flags & EV_OOBAND)
        printf(" ev.flags\t: EV_OOBAND\n");
    if (ev.flags & EV_EOF)
        printf(" ev.flags\t: EV_EOF\n");

    printf(" ev.data\t: %lld\n", ev.data);
    printf(" ev.udata\t: %llx, %llu\n", ev.udata, ev.udata);
}
