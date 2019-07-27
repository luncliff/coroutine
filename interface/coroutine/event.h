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

#include <system_error>
#include <atomic>

#if __has_include(<coroutine/frame.h>)
#include <coroutine/frame.h>
#elif __has_include(<experimental/coroutine>) // C++ 17
#include <experimental/coroutine>
#elif __has_include(<coroutine>) // C++ 20
#include <coroutine>
#else
#error "expect header <experimental/coroutine> or <coroutine/frame.h>"
#endif

#if __has_include(<Windows.h>) // ... activate VC++ based features ...

// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
// clang-format on

namespace coro {
using namespace std;
using namespace std::experimental;

//  Awaitable event with thread pool. Only for one-time usage
//  This type is not for rvalue reference. This design is unavoidable
//   since `ptp_event` uses INFINITE wait. Therefore, its user must make
//   sure one of `SetEvent` or `cancel` will happen in the future
class ptp_event final : no_copy_move {
    HANDLE wo{}; // wait object

  private:
    // WAITORTIMERCALLBACK
    static void __stdcall wait_on_thread_pool(PVOID, BOOLEAN);

    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void on_suspend(coroutine_handle<void>) noexcept(false);
    _INTERFACE_ auto on_resume() noexcept -> uint32_t; // error code
  public:
    _INTERFACE_ explicit ptp_event(HANDLE target) noexcept(false);
    _INTERFACE_ ~ptp_event() noexcept;

    _INTERFACE_ void cancel() noexcept;

    bool await_ready() noexcept {
        return this->is_ready();
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

//  Awaitable event type.
//  If the event object is signaled(`set`), the library will yield suspended
//  coroutine via `signaled_event_tasks` function.
//  If it is signaled before `co_await`, it will return `true` for
//  `await_ready` so the coroutine can bypass suspension steps. The event
//  object can be `co_await`ed multiple times.
class event final {
  public:
    using task = coroutine_handle<void>;

  private:
    uint64_t state;

  private:
    _INTERFACE_ void on_suspend(task) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void on_resume() noexcept;

  public:
    _INTERFACE_ event() noexcept(false);
    _INTERFACE_ ~event() noexcept;

    _INTERFACE_ void set() noexcept(false);

    bool await_ready() const noexcept {
        return this->is_ready();
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        return this->on_suspend(coro);
    }
    void await_resume() noexcept {
        return this->on_resume();
    }
};

//  Enumerate all suspended coroutines that are waiting for signaled events.
_INTERFACE_
auto signaled_event_tasks() noexcept(false) -> enumerable<event::task>;

} // namespace coro
#endif
#endif // COROUTINE_AWAITABLE_EVENT_H
