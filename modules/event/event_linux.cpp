//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>
#include <system_error>

#include "event_poll.h"

namespace coro {

// signaled event list. it's badly named to prevent possible collision
event_poll_t selist{};

//
//  We are going to combine file descriptor and state bit
//
//  On x86 system,
//    this won't matter since `int` is 32 bit.
//    we can safely use msb for state indicator.
//
//  On x64 system,
//    this might be a hazardous since the value of `eventfd` can be corrupted.
//    **Normally** descriptor in Linux system grows from 3, so it is highly
//    possible to reach system limitation before the value consumes all 63 bit.
//
constexpr uint64_t emask = 1ULL << 63;

//  the msb(most significant bit) will be ...
//   1 if the fd is signaled,
//   0 on the other case
bool is_signaled(uint64_t state) noexcept {
    return emask & state; // msb is 1?
}
int64_t get_eventfd(uint64_t state) noexcept {
    return static_cast<int64_t>(~emask & state);
}
uint64_t make_signaled(int64_t efd) noexcept(false) {
    // signal the eventfd...
    //  the message can be any value
    //  since the purpose of it is to trigger the epoll
    //  we won't care about the internal counter of the eventfd
    auto sz = write(efd, &efd, sizeof(efd));
    if (sz == -1)
        throw system_error{errno, system_category(), "write"};

    return emask | static_cast<uint64_t>(efd);
}

auto_reset_event::auto_reset_event() noexcept(false) : state{} {
    const auto fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1)
        throw system_error{errno, system_category(), "eventfd"};

    this->state = fd; // start with unsignaled state
}
auto_reset_event::~auto_reset_event() noexcept {
    // if already closed, fd == 0
    if (auto fd = get_eventfd(state))
        close(fd);
}

void auto_reset_event::set() noexcept(false) {
    // already signaled. nothing to do...
    if (is_signaled(state))
        // !!! under the race condition, this check is not safe !!!
        return;

    auto fd = get_eventfd(state);
    state = make_signaled(fd); // if it didn't throwed
                               //  it's signaled state from now
}
bool auto_reset_event::is_ready() const noexcept {
    return is_signaled(state);
}
void auto_reset_event::reset() noexcept {
    // make unsignaled state
    this->state = static_cast<decltype(state)>(get_eventfd(state));
}

// Reference
//  https://github.com/grpc/grpc/blob/master/src/core/lib/iomgr/is_epollexclusive_available.cc
void auto_reset_event::on_suspend(coroutine_handle<void> t) noexcept(false) {
    // just care if there was `write` for the eventfd
    //  when it happens, coroutine handle will be forwarded by epoll
    epoll_event req{};
    req.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
    req.data.ptr = t.address();

    // throws if `epoll_ctl` fails
    selist.try_add(get_eventfd(state), req);
}

auto signaled_event_tasks() noexcept(false)
    -> coro::enumerable<coroutine_handle<void>> {
    coroutine_handle<void> t{};

    // notice that the timeout is zero
    for (auto e : selist.wait(0)) {

        // see also: `make_signaled`, `auto_reset_event::on_suspend`
        // we don't care about the internal counter.
        //  just receive the coroutine handle
        t = coroutine_handle<void>::from_address(e.data.ptr);
        // ensure we can resume it.
        if (t.done())
            // todo: check only for debug mode?
            throw runtime_error{"coroutine_handle<void> is already done state"};

        co_yield t;
    }
    co_return;
}

} // namespace coro
