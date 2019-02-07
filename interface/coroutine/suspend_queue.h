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

#ifndef COROUTINE_SUSPEND_QUEUE_H
#define COROUTINE_SUSPEND_QUEUE_H

#include <coroutine/frame.h>

using coroutine_task_t = std::experimental::coroutine_handle<void>;

class suspend_queue;

// - Note
//      awaitable for suspend queue
class suspend_wait final
{
    friend class suspend_queue;

    suspend_queue& sq;

  public:
    ~suspend_wait() noexcept = default;
    suspend_wait(suspend_wait&&) noexcept = default;
    suspend_wait& operator=(suspend_wait&&) noexcept = default;
    suspend_wait(const suspend_wait&) noexcept = default;
    suspend_wait& operator=(const suspend_wait&) noexcept = default;

  private:
    _INTERFACE_ explicit suspend_wait(suspend_queue& q) noexcept;

  public:
    _INTERFACE_ bool ready() const noexcept;
    _INTERFACE_ void suspend(coroutine_task_t coro) noexcept(false);
    _INTERFACE_ void resume() noexcept;

#pragma warning(disable : 4505)
    bool await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(coroutine_task_t coro) noexcept(false)
    {
        return this->suspend(coro);
    }
    void await_resume() noexcept
    {
        return this->resume();
    }
#pragma warning(default : 4505)
};

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

    _INTERFACE_ auto wait() noexcept -> suspend_wait;

    _INTERFACE_ void push(coroutine_task_t coro) noexcept(false);
    _INTERFACE_ bool try_pop(coroutine_task_t& coro) noexcept;
};

#endif // COROUTINE_SUSPEND_QUEUE_H
