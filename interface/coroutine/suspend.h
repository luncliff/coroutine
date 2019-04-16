// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Inferface to suspend current coroutines and fetch them
//
// ---------------------------------------------------------------------------
#pragma once
// clang-format off
#ifdef USE_STATIC_LINK_MACRO // ignore macro declaration in static build
#   define _INTERFACE_
#   define _HIDDEN_
#else 
#   if defined(_MSC_VER) // MSVC
#       define _HIDDEN_
#       ifdef _WINDLL
#           define _INTERFACE_ __declspec(dllexport)
#       else
#           define _INTERFACE_ __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) || defined(__clang__)
#       define _INTERFACE_ __attribute__((visibility("default")))
#       define _HIDDEN_ __attribute__((visibility("hidden")))
#   else
#       error "unexpected compiler"
#   endif // compiler check
#endif
// clang-format on

#ifndef COROUTINE_SUSPEND_HELPER_TYPES_H
#define COROUTINE_SUSPEND_HELPER_TYPES_H

#include <coroutine/frame.h>
#include <memory>

namespace coro
{
using namespace std::experimental;

// - Note
//      Provide interface for manual resume operation after
//      being used as an argument of `co_await` by reference
class suspend_hook final : public coroutine_handle<void>, public suspend_always
{
  public:
    // - Note
    //    Override `suspend_always::await_suspend`
    void await_suspend(coroutine_handle<void> coro) noexcept
    {
        coroutine_handle<void>& self = *this;
        self = coro;
    }
};
static_assert(sizeof(suspend_hook) == sizeof(coroutine_handle<void>));

class limited_lock_queue
{
  public:
    using value_type = void*;

  public:
    static constexpr bool is_lock_free() noexcept
    {
        return false;
    }

  public:
    limited_lock_queue() noexcept = default;
    limited_lock_queue(const limited_lock_queue&) = delete;
    limited_lock_queue(limited_lock_queue&&) = delete;
    limited_lock_queue& operator=(const limited_lock_queue&) = delete;
    limited_lock_queue& operator=(limited_lock_queue&&) = delete;
    _INTERFACE_ virtual ~limited_lock_queue() noexcept = default;

  public:
    _INTERFACE_ virtual bool wait_push(value_type) noexcept(false) = 0;
    _INTERFACE_ virtual bool wait_pop(value_type&) noexcept(false) = 0;
    _INTERFACE_ virtual void close() noexcept = 0;
    _INTERFACE_ virtual bool is_closed() const noexcept = 0;
    // _INTERFACE_ virtual void open() = 0;
    _INTERFACE_ virtual bool is_empty() const noexcept = 0;
    _INTERFACE_ virtual bool is_full() const noexcept = 0;
};

_INTERFACE_ auto make_lock_queue() -> std::unique_ptr<limited_lock_queue>;

// - Note
//      Return an awaitable that enqueue the coroutine
//      Relay code will be generated with this header to minimize dllexport
//      functions
inline auto push_to(limited_lock_queue& queue) noexcept
{
    // awaitable for the queue
    class redirect_to final : public suspend_always
    {
        limited_lock_queue& sq;

      public:
        explicit redirect_to(limited_lock_queue& queue) noexcept : sq{queue}
        {
        }
        // override `suspend_always::await_suspend`
        void await_suspend(coroutine_handle<void> coro) noexcept(false)
        {
            sq.wait_push(coro.address());
        }
    };
    static_assert(sizeof(redirect_to) == sizeof(limited_lock_queue*));
    return redirect_to{queue};
}

inline auto pop_from(limited_lock_queue& queue) noexcept
    -> coroutine_handle<void>
{
    void* ptr = nullptr;
    queue.wait_pop(ptr);
    return coroutine_handle<void>::from_address(ptr);
}

} // namespace coro

#endif // COROUTINE_SUSPEND_HELPER_TYPES_H
