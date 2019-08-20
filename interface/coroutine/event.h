//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
// clang-format off
#if defined(FORCE_STATIC_LINK)
#   define _INTERFACE_
#   define _HIDDEN_
#elif defined(_MSC_VER) // MSVC or clang-cl
#   define _HIDDEN_
#   ifdef _WINDLL
#       define _INTERFACE_ __declspec(dllexport)
#   else
#       define _INTERFACE_ __declspec(dllimport)
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define _INTERFACE_ __attribute__((visibility("default")))
#   define _HIDDEN_ __attribute__((visibility("hidden")))
#else
#   error "unexpected linking configuration"
#endif
// clang-format on

#ifndef COROUTINE_AWAITABLE_EVENT_H
#define COROUTINE_AWAITABLE_EVENT_H

#if __has_include(<Windows.h>) // ... activate VC++ based features ...
#include <coroutine/return.h>

namespace coro {
using namespace std;
using namespace std::experimental;

using win_handle_t = void*;

//  Awaitable event type over Win32 thread pool
//
//  Its object can be `co_await`ed only once.
//  The purpose of such design is to encourage use of short functions rather
//  than containing multiple objects in the function's body
//
//  It uses INFINITE wait. So user must sure one of `SetEvent(hEvent)`
//  or `sc.cancel()` happens after `co_await`.
//
//  set_or_cancel sc{hEvent};
//  co_await sc;
//
class set_or_cancel final {
    win_handle_t hobject; // object for wait register/unregister

  public:
    set_or_cancel(const set_or_cancel&) = delete;
    set_or_cancel(set_or_cancel&&) = delete;
    set_or_cancel& operator=(const set_or_cancel&) = delete;
    set_or_cancel& operator=(set_or_cancel&&) = delete;

    _INTERFACE_ explicit set_or_cancel(win_handle_t target) noexcept(false);
    _INTERFACE_ ~set_or_cancel() noexcept;

  private:
    _INTERFACE_ void on_suspend(coroutine_handle<void>) noexcept(false);
    _INTERFACE_ auto on_resume() noexcept -> uint32_t; // error code
  public:
    _INTERFACE_ auto cancel() noexcept -> uint32_t;

    constexpr bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        return this->on_suspend(coro);
    }
    auto await_resume() noexcept {
        return this->on_resume();
    }
};

} // namespace coro

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
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

    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void on_suspend(coroutine_handle<void>) noexcept(false);
    _INTERFACE_ void reset() noexcept;

  public:
    _INTERFACE_ auto_reset_event() noexcept(false);
    _INTERFACE_ ~auto_reset_event() noexcept;

    _INTERFACE_ void set() noexcept(false);

    bool await_ready() const noexcept {
        return this->is_ready();
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        return this->on_suspend(coro);
    }
    void await_resume() noexcept {
        return this->reset(); // automatically reset on resume
    }
};

//  Enumerate all suspended coroutines that are waiting for signaled events.
_INTERFACE_
auto signaled_event_tasks() noexcept(false)
    -> enumerable<coroutine_handle<void>>;

} // namespace coro
#endif
#endif // COROUTINE_AWAITABLE_EVENT_H
