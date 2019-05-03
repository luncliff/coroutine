// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>

#include "./event_poll.h"

static_assert(sizeof(ssize_t) <= sizeof(int64_t));
using namespace std;
using namespace std::chrono;

event_poll_t inbound{}, outbound{};

auto wait_io_tasks(nanoseconds timeout) noexcept(false)
    -> coro::enumerable<io_task_t> {
    const int half_time = duration_cast<milliseconds>(timeout).count() / 2;
    io_task_t task{};

    for (auto event : inbound.wait(half_time))
        co_yield task = io_task_t::from_address(event.data.ptr);
    for (auto event : outbound.wait(half_time))
        co_yield task = io_task_t::from_address(event.data.ptr);
}

bool io_work_t::ready() const noexcept {
    auto sd = this->handle;
    // non blocking operation is expected
    // going to suspend
    if (fcntl(sd, F_GETFL, 0) & O_NONBLOCK)
        return false;

    // not configured. return true
    // and bypass to the blocking I/O
    return true;
}

uint32_t io_work_t::error() const noexcept {
    return gsl::narrow_cast<uint32_t>(this->internal);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto send_to(uint64_t sd, const sockaddr_in& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {
    work.handle = sd;
    work.ep = reinterpret_cast<endpoint_t*>(
        const_cast<sockaddr_in*>(addressof(remote)));
    work.offset = sizeof(sockaddr_in);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

auto send_to(uint64_t sd, const sockaddr_in6& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {
    work.handle = sd;
    work.ep = reinterpret_cast<endpoint_t*>(
        const_cast<sockaddr_in6*>(addressof(remote)));
    work.offset = sizeof(sockaddr_in6);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

void io_send_to::suspend(io_task_t rh) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    outbound.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_send_to::resume() noexcept {
    auto sd = this->handle;
    auto addr = addressof(this->ep->addr);
    auto addrlen = static_cast<socklen_t>(this->offset);
    auto& errc = this->internal;
    auto sz = sendto(sd, buffer.data(), buffer.size_bytes(), //
                     0, addr, addrlen);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto recv_from(uint64_t sd, sockaddr_in& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {
    work.handle = sd;
    work.ep = reinterpret_cast<endpoint_t*>(addressof(remote));
    work.offset = sizeof(sockaddr_in);
    work.buffer = buffer;
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

auto recv_from(uint64_t sd, sockaddr_in6& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {
    work.handle = sd;
    work.ep = reinterpret_cast<endpoint_t*>(addressof(remote));
    work.offset = sizeof(sockaddr_in6);
    work.buffer = buffer;
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

void io_recv_from::suspend(io_task_t rh) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    inbound.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_recv_from::resume() noexcept {
    auto sd = this->handle;
    auto addr = addressof(this->ep->addr);
    auto addrlen = static_cast<socklen_t>(this->offset);
    auto& errc = this->internal;
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, addr, addressof(addrlen));
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto send_stream(uint64_t sd, buffer_view_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_send& {
    static_assert(sizeof(socklen_t) == sizeof(uint32_t));
    work.handle = sd;
    work.internal = flag;
    work.buffer = buffer;
    return *reinterpret_cast<io_send*>(addressof(work));
}

void io_send::suspend(io_task_t rh) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    outbound.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_send::resume() noexcept {
    auto sd = this->handle;
    auto flag = this->internal;
    auto& errc = this->internal;
    const auto sz = send(sd, buffer.data(), buffer.size_bytes(), flag);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto recv_stream(uint64_t sd, buffer_view_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_recv& {
    static_assert(sizeof(socklen_t) == sizeof(uint32_t));
    work.handle = sd;
    work.internal = flag;
    work.buffer = buffer;
    return *reinterpret_cast<io_recv*>(addressof(work));
}

void io_recv::suspend(io_task_t rh) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = rh.address();

    inbound.try_add(sd, req);
}

int64_t io_recv::resume() noexcept {
    auto sd = this->handle;
    auto flag = this->internal;
    auto& errc = this->internal;
    const auto sz = recv(sd, buffer.data(), buffer.size_bytes(), flag);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

errc peer_name(uint64_t sd, sockaddr_in6& ep) noexcept {
    socklen_t len = sizeof(sockaddr_in6);
    errc ec{};

    if (getpeername(static_cast<int>(sd), reinterpret_cast<sockaddr*>(&ep),
                    &len))
        ec = errc{errno};
    return ec;
}

errc sock_name(uint64_t sd, sockaddr_in6& ep) noexcept {
    socklen_t len = sizeof(sockaddr_in6);
    errc ec{};

    if (getsockname(static_cast<int>(sd), reinterpret_cast<sockaddr*>(&ep),
                    &len))
        ec = errc{errno};
    return ec;
}
