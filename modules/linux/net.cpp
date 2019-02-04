// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <sys/epoll.h>

static_assert(sizeof(ssize_t) <= sizeof(int64_t));

// use 2 page for generator
const size_t max_event_size = 2 * getpagesize() / sizeof(epoll_event);

auto ep_fd = epoll_create1(EPOLL_CLOEXEC);
auto ep_dtor = gsl::finally([]() noexcept { close(ep_fd); });
auto ep_events = std::make_unique<epoll_event[]>(max_event_size);

void remove_from_epfd(int sd) noexcept
{
    epoll_event req{}; // just prevent non-null input
    auto ec = epoll_ctl(ep_fd, EPOLL_CTL_DEL, sd, &req);
    assert(ec == 0);
}

void try_add_to_epfd(int sd, epoll_event& req) noexcept(false)
{
    int op = EPOLL_CTL_ADD;
TRY_OP:
    auto ec = epoll_ctl(ep_fd, op, sd, &req);
    if (errno == EEXIST)
    {
        op = EPOLL_CTL_MOD;
        goto TRY_OP;
    }
    if (ec == -1)
        throw std::system_error{errno, std::system_category(), "epoll_ctl"};
}

auto fetch_io_tasks() noexcept(false) -> enumerable<coroutine_task_t>
{
    constexpr auto timeout = -1;
    auto count = epoll_wait(ep_fd, ep_events.get(), max_event_size, timeout);
    if (count == -1)
        throw std::system_error{errno, std::system_category(), "epoll_wait"};

    for (auto i = 0; i < count; ++i)
    {
        auto& ev = ep_events[i];
        auto task = coroutine_task_t::from_address(ev.data.ptr);
        co_yield task;
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

auto send_to(int sd, const sockaddr_in& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    work.sd = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(std::addressof(work));
}

void io_send_to::suspend(coroutine_task_t rh) noexcept(false)
{
    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    try_add_to_epfd(this->sd, req); // throws if epoll_ctl fails
}

int64_t io_send_to::resume() noexcept
{
    auto sz = sendto(sd, buffer.data(), buffer.size_bytes(),   //
                     0, reinterpret_cast<const sockaddr*>(to), //
                     sizeof(sockaddr_in));
    return sz;
}
auto recv_from(int sd, sockaddr_in& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    work.sd = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;

    return *reinterpret_cast<io_recv_from*>(std::addressof(work));
}

void io_recv_from::suspend(coroutine_task_t rh) noexcept(false)
{
    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    try_add_to_epfd(this->sd, req); // throws if epoll_ctl fails
}

int64_t io_recv_from::resume() noexcept
{
    // recv event is detected
    socklen_t length = sizeof(sockaddr_in);
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, reinterpret_cast<sockaddr*>(from),
                       std::addressof(length));
    // return follows that recvfrom
    return sz;
}
