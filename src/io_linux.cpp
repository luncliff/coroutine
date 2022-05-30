/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <chrono>

#include <coroutine/linux.h>
#include <coroutine/net.h>

static_assert(sizeof(ssize_t) <= sizeof(int64_t));
using namespace std;
using namespace std::chrono;

namespace coro {

epoll_owner iep{}, oep{}; // inbound, outbound

void poll_net_tasks(uint64_t nano) noexcept(false) {
    const auto half_time = duration_cast<milliseconds>(nanoseconds{nano} / 2);
    // event buffer for this poll
    constexpr auto buf_sz = 30u;
    auto buf = make_unique<epoll_event[]>(buf_sz);
    // resume inbound coroutines
    {
        auto count = iep.wait(half_time.count(), {buf.get(), buf_sz});
        for (auto i = 0u; i < count; ++i)
            if (auto coro =
                    coroutine_handle<void>::from_address(buf[i].data.ptr))
                coro.resume();
    }
    // resume outbound coroutines
    {
        auto count = oep.wait(half_time.count(), {buf.get(), buf_sz});
        for (auto i = 0u; i < count; ++i)
            if (auto coro =
                    coroutine_handle<void>::from_address(buf[i].data.ptr))
                coro.resume();
    }
}

bool io_work_t::ready() const noexcept {
    auto sd = this->handle;
    // non blocking operation is expected going to suspend
    if (fcntl(sd, F_GETFL, 0) & O_NONBLOCK)
        return false;
    // not configured. return `true` and bypass to the blocking I/O
    return true;
}

uint32_t io_work_t::error() const noexcept {
    return gsl::narrow_cast<uint32_t>(this->internal);
}

auto send_to(uint64_t sd, const sockaddr_in& remote, io_buffer_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {
    work.handle = sd;
    work.ptr = const_cast<sockaddr_in*>(addressof(remote));
    work.internal_high = sizeof(sockaddr_in);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

auto send_to(uint64_t sd, const sockaddr_in6& remote, io_buffer_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {
    work.handle = sd;
    work.ptr = const_cast<sockaddr_in6*>(addressof(remote));
    work.internal_high = sizeof(sockaddr_in6);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

void io_send_to::suspend(coroutine_handle<void> coro) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = coro.address();

    oep.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_send_to::resume() noexcept {
    auto sd = this->handle;
    auto addr = reinterpret_cast<sockaddr*>(this->ptr);
    auto addrlen = static_cast<socklen_t>(this->internal_high);
    auto& errc = this->internal;
    auto sz = sendto(sd, buffer.data(), buffer.size_bytes(), //
                     0, addr, addrlen);
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

auto recv_from(uint64_t sd, sockaddr_in& remote, io_buffer_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {
    work.handle = sd;
    work.ptr = addressof(remote);
    work.internal_high = sizeof(sockaddr_in);
    work.buffer = buffer;
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

auto recv_from(uint64_t sd, sockaddr_in6& remote, io_buffer_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {
    work.handle = sd;
    work.ptr = addressof(remote);
    work.internal_high = sizeof(sockaddr_in6);
    work.buffer = buffer;
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

void io_recv_from::suspend(coroutine_handle<void> coro) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = coro.address();

    iep.try_add(sd, req); // throws if epoll_ctl fails
}

int64_t io_recv_from::resume() noexcept {
    auto sd = this->handle;
    auto addr = reinterpret_cast<sockaddr*>(this->ptr);
    auto addrlen = static_cast<socklen_t>(this->internal_high);
    auto& errc = this->internal;
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, addr, addressof(addrlen));
    // update error code upon i/o failure
    errc = sz < 0 ? errno : 0;
    return sz;
}

auto send_stream(uint64_t sd, io_buffer_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_send& {
    static_assert(sizeof(socklen_t) == sizeof(uint32_t));
    work.handle = sd;
    work.internal = flag;
    work.buffer = buffer;
    return *reinterpret_cast<io_send*>(addressof(work));
}

void io_send::suspend(coroutine_handle<void> coro) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
    req.data.ptr = coro.address();

    oep.try_add(sd, req); // throws if epoll_ctl fails
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

auto recv_stream(uint64_t sd, io_buffer_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_recv& {
    static_assert(sizeof(socklen_t) == sizeof(uint32_t));
    work.handle = sd;
    work.internal = flag;
    work.buffer = buffer;
    return *reinterpret_cast<io_recv*>(addressof(work));
}

void io_recv::suspend(coroutine_handle<void> coro) noexcept(false) {
    auto sd = this->handle;
    auto& errc = this->internal;
    errc = 0;

    epoll_event req{};
    req.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
    req.data.ptr = coro.address();

    iep.try_add(sd, req);
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

} // namespace coro
