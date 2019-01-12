// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Thread switching for the coroutines
//
//  Reference
//      - Windows via C/C++ (5th Edition)
//      - MSDN | Thread Pool API
//      - CppCon 2016 | Putting Coroutines to Work with the Windows Runtime
//          by Kenny Kerr & James McNellis
//
// ---------------------------------------------------------------------------
#pragma once
#ifndef COROUTINE_THREAD_SWITCHING_H
#define COROUTINE_THREAD_SWITCHING_H

#include <coroutine/frame.h>
#include <coroutine/sync.h>

class scheduler_t final
{
  public:
    using coroutine_task = std::experimental::coroutine_handle<void>;
    using duration = std::chrono::microseconds;

  private:
    const uint64_t storage[4]{};

  public:
    scheduler_t(scheduler_t const&) = delete;
    scheduler_t(scheduler_t&&) = delete;
    scheduler_t& operator=(scheduler_t const&) = delete;
    scheduler_t& operator=(scheduler_t&&) = delete;
    _INTERFACE_ scheduler_t() = default;
    _INTERFACE_ ~scheduler_t() = default;

    _INTERFACE_ void close() noexcept;
    _INTERFACE_ bool closed() const noexcept;
    [[nodiscard]] _INTERFACE_ //
        auto
        wait(duration d) noexcept(false) -> coroutine_task;
};

// - Note
//      Routine switching to another thread with MSVC Coroutine
//      and Windows Thread Pool
class switch_to final
{
  public:
    using coroutine_task = std::experimental::coroutine_handle<void>;

  private:
    // reserve enough size to provide platform compatibility
    const uint64_t storage[4]{};

  public:
    switch_to(switch_to&&) noexcept = delete;
    switch_to& operator=(switch_to&&) noexcept = delete;
    switch_to(const switch_to&) noexcept = delete;
    switch_to& operator=(const switch_to&) noexcept = delete;
    _INTERFACE_ switch_to() noexcept(false);
    _INTERFACE_ ~switch_to() noexcept;

    _INTERFACE_ auto scheduler() noexcept(false) -> scheduler_t&;

    _INTERFACE_ bool ready() const noexcept;
    _INTERFACE_ void suspend(coroutine_task coro) noexcept(false);
    _INTERFACE_ void resume() noexcept;

    // Lazy code generation in library user code
#pragma warning(disable : 4505)
    bool await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend( //
        std::experimental::coroutine_handle<void> rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    void await_resume() noexcept
    {
        return this->resume();
    }
#pragma warning(default : 4505)
};

#endif // COROUTINE_THREAD_SWITCHING_H
