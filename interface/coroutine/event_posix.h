/**
 * @file event_posix.h
 * @brief Awaitable type over system's event object
 * @author github.com/luncliff <luncliff@gmail.com>
 * @copyright CC BY 4.0
 */
#pragma once
#ifndef COROUTINE_AWAITABLE_EVENT_H
#define COROUTINE_AWAITABLE_EVENT_H

#include <coroutine/frame.h>

/**
 * @defgroup Event
 * Helper types to apply `co_await` for event objects. 
 * For Windows, the implementation uses Win32 thread pool.
 * For Linux, it uses Epoll.
 * For Darwin, it uses Kqueue.
 */

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <coroutine/yield.hpp>

namespace coro {
using namespace std;
using namespace std::experimental;

//  Awaitable event type over `eventfd` & `epoll`
//
//  For Darwin(Apple) platform, it uses UNIX domain socket and `kqueue`
//  Its object can be `co_await`ed multiple times
//
//  If the object is signaled(`set`), the library will yield suspended coroutine
//  through `signaled_event_tasks` function.
//
//  If it is signaled before `co_await`,
//  its `await_ready` will return `true` so the `co_await` can bypass the
//  suspension steps
//
class auto_reset_event final {
    uint64_t state; // works with stateful implementation

  private:
    auto_reset_event(const auto_reset_event&) = delete;
    auto_reset_event(auto_reset_event&&) = delete;
    auto_reset_event& operator=(const auto_reset_event&) = delete;
    auto_reset_event& operator=(auto_reset_event&&) = delete;

    bool is_ready() const noexcept;
    void on_suspend(coroutine_handle<void>) noexcept(false);
    void reset() noexcept(false);

  public:
    auto_reset_event() noexcept(false);
    ~auto_reset_event() noexcept;

    void set() noexcept(false);

    bool await_ready() const noexcept {
        return this->is_ready();
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        return this->on_suspend(coro);
    }
    void await_resume() noexcept(false) {
        return this->reset(); // automatically reset on resume
    }
};

//  Enumerate all suspended coroutines that are waiting for signaled events.

auto signaled_event_tasks() noexcept(false)
    -> enumerable<coroutine_handle<void>>;

} // namespace coro
#endif
#endif // COROUTINE_AWAITABLE_EVENT_H
