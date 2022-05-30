/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <chrono>

#include <coroutine/net.h>
#include <coroutine/unix.h>

static_assert(sizeof(ssize_t) <= sizeof(int64_t));
using namespace std;
using namespace std::chrono;

namespace coro {

kqueue_owner netkq{};

using net_callback_t = void (*)(void* ctx, coroutine_handle<void> coro);

void poll_net_tasks(const timespec& wait_time, //
                    net_callback_t callback, void* ctx) noexcept(false) {

    constexpr auto buf_size = 30u;
    auto buf = make_unique<kevent64_s[]>(buf_size);

    const auto count = netkq.events(wait_time, {buf.get(), buf_size});
    for (auto i = 0; i < count; ++i) {
        auto* work = reinterpret_cast<io_work_t*>(buf[i].udata);
        callback(ctx, work->task);
    }
}

void resume_net_task(void*, coroutine_handle<void> coro) noexcept(false) {
    return coro.resume();
}

void poll_net_tasks(uint64_t nano) noexcept(false) {
    auto timeout = nanoseconds{nano};
    const auto sec = duration_cast<seconds>(timeout);
    const timespec wait_time{
        .tv_sec = sec.count(),
        .tv_nsec = (timeout - sec).count(),
    };
    return poll_net_tasks(wait_time, resume_net_task, nullptr);
}

bool io_work_t::ready() const noexcept {
    const auto sd = this->handle;
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

void io_send_to::suspend(coroutine_handle<void> rh) noexcept(false) {
    static_assert(sizeof(void*) <= sizeof(uint64_t));
    task = rh;

    // one-shot, write registration (edge-trigger)
    kevent64_s req{};
    req.ident = this->handle;
    req.filter = EVFILT_WRITE;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    netkq.change(req);
}

int64_t io_send_to::resume() noexcept {
    auto sd = this->handle;
    auto addr = reinterpret_cast<sockaddr*>(this->ptr);
    auto addrlen = static_cast<socklen_t>(this->internal_high);
    auto& errc = this->internal;
    auto sz = sendto(sd, buffer.data(), buffer.size_bytes(), //
                     0, addr, addrlen);
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

void io_recv_from::suspend(coroutine_handle<void> rh) noexcept(false) {
    static_assert(sizeof(void*) <= sizeof(uint64_t));

    task = rh;
    // system operation
    kevent64_s req{};
    req.ident = this->handle;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;

    // it is possible to pass `rh` for the user data,
    // but will pass this object to support
    // receiving some values from `wait_io_tasks`
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    netkq.change(req);
}

int64_t io_recv_from::resume() noexcept {
    auto sd = this->handle;
    auto addr = reinterpret_cast<sockaddr*>(this->ptr);
    auto addrlen = static_cast<socklen_t>(this->internal_high);
    auto& errc = this->internal;
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, addr, addressof(addrlen));
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

void io_send::suspend(coroutine_handle<void> rh) noexcept(false) {
    static_assert(sizeof(void*) <= sizeof(uint64_t));
    task = rh;

    // one-shot, write registration (edge-trigger)
    kevent64_s req{};
    req.ident = this->handle;
    req.filter = EVFILT_WRITE;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    netkq.change(req);
}

int64_t io_send::resume() noexcept {
    auto sd = this->handle;
    auto flag = this->internal;
    auto& errc = this->internal;
    const auto sz = send(sd, buffer.data(), buffer.size_bytes(), flag);
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

void io_recv::suspend(coroutine_handle<void> rh) noexcept(false) {
    static_assert(sizeof(void*) <= sizeof(uint64_t));

    task = rh;
    // system operation
    kevent64_s req{};
    req.ident = this->handle;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    netkq.change(req);
}

int64_t io_recv::resume() noexcept {
    auto sd = this->handle;
    auto flag = this->internal;
    auto& errc = this->internal;
    const auto sz = recv(sd, buffer.data(), buffer.size_bytes(), flag);
    errc = sz < 0 ? errno : 0;
    return sz;
}

} // namespace coro
