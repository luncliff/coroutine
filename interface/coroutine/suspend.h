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

#ifdef USE_STATIC_LINK_MACRO // clang-format off
#   define _INTERFACE_
#   define _HIDDEN_
#else // ignore macro declaration in static build
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
#endif // clang-format on

#ifndef COROUTINE_SUSPEND_HELPER_TYPES_H
#define COROUTINE_SUSPEND_HELPER_TYPES_H

#include <coroutine/frame.h>

using coroutine_task_t = std::experimental::coroutine_handle<void>;

// - Note
//      Provide interface for manual resume operation after
//      being used as an argument of `co_await` *by reference*
class suspend_hook final : public std::experimental::suspend_always,
                           public coroutine_task_t
{
  public:
    void await_suspend(coroutine_task_t rh) noexcept
    {
        coroutine_task_t& coro = *this;
        coro = std::move(rh); // update frame value
    }
};
static_assert(sizeof(suspend_hook) == sizeof(coroutine_task_t));

// - Note
//      Interface to suspend coroutines and fetch them *manually*.
class suspend_queue final
{
    // reserve enough size to provide platform compatibility
    const uint64_t storage[8]{};

  public:
    suspend_queue(suspend_queue&&) noexcept = delete;
    suspend_queue& operator=(suspend_queue&&) noexcept = delete;
    suspend_queue(const suspend_queue&) noexcept = delete;
    suspend_queue& operator=(const suspend_queue&) noexcept = delete;

    _INTERFACE_ suspend_queue() noexcept(false);
    _INTERFACE_ ~suspend_queue() noexcept;

    _INTERFACE_ void push(coroutine_task_t coro) noexcept(false);
    _INTERFACE_ bool try_pop(coroutine_task_t& coro) noexcept;

    // - Note
    //      Return an awaitable that enqueue the coroutine
    //      Relay code will be generated with this header to minimize dllexport
    //      functions
    auto wait() noexcept
    {
        // awaitable for suspend queue
        class redirect_to final : public std::experimental::suspend_always
        {
            suspend_queue& sq;

          public:
            redirect_to(suspend_queue& q) noexcept : sq{q}
            {
            }
            void await_suspend(coroutine_task_t coro) noexcept(false)
            {
                return sq.push(coro);
            }
        };
        static_assert(sizeof(redirect_to) == sizeof(void*));
        return redirect_to{*this};
    }
};

#endif // COROUTINE_SUSPEND_HELPER_TYPES_H
