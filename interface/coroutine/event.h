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
#include <Windows.h>

namespace coro {
using namespace std;
using namespace std::experimental;

//  Awaitable event type over Win32 thread pool.
//  The `ptp_event` uses INFINITE wait.
//  Therefore, its user must sure one of `SetEvent` or `cancel`
//  after its `co_await`
class ptp_event final {
    HANDLE wo{}; // wait object

  private:
    ptp_event(const ptp_event&) = delete;
    ptp_event(ptp_event&&) = delete;
    ptp_event& operator=(const ptp_event&) = delete;
    ptp_event& operator=(ptp_event&&) = delete;

    // WAITORTIMERCALLBACK
    static void __stdcall wait_on_thread_pool(HANDLE, BOOLEAN);

    _INTERFACE_ void on_suspend(coroutine_handle<void>) noexcept(false);
    _INTERFACE_ auto on_resume() noexcept -> uint32_t; // error code
  public:
    _INTERFACE_ explicit ptp_event(HANDLE target) noexcept(false);
    _INTERFACE_ ~ptp_event() noexcept;

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

//  Awaitable event type over eventfd and unix socket
//  If the event object is signaled(`set`),
//  the library will yield suspended coroutine through `signaled_event_tasks`
//  function.
//  If it is signaled before `co_await`,
//  its `await_ready` will return `true` so the coroutine can bypass suspension.
//  It can be `co_await`ed multiple times.
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
