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

_INTERFACE_
bool peek_switched( //
    std::experimental::coroutine_handle<void>& rh) noexcept(false);

// - Note
//      Routine switching to another thread with MSVC Coroutine
//      and Windows Thread Pool
class switch_to final
{
    template <typename Promise>
    using coroutine_handle = std::experimental::coroutine_handle<Promise>;

  private:
    // reserve enough size to provide platform compatibility
    const uint64_t storage[4]{};

  private:
    switch_to(switch_to&&) noexcept = delete;
    switch_to& operator=(switch_to&&) noexcept = delete;
    switch_to(const switch_to&) noexcept = delete;
    switch_to& operator=(const switch_to&) noexcept = delete;

  public:
    _INTERFACE_ explicit //
        switch_to(thread_id_t target = thread_id_t{0}) noexcept(false);
    _INTERFACE_ ~switch_to() noexcept;

  public:
    _INTERFACE_ bool ready() const noexcept;
    _INTERFACE_ void suspend(coroutine_handle<void> rh) noexcept(false);
    _INTERFACE_ void resume() noexcept;

    // Lazy code generation in library user code
#pragma warning(disable : 4505)
    bool await_ready() const noexcept
    {
        // redirect
        return this->ready();
    }
    void await_suspend(coroutine_handle<void> rh) noexcept(false)
    {
        // redirect
        return this->suspend(rh);
    }
    void await_resume() noexcept
    {
        // redirect
        return this->resume();
    }
#pragma warning(default : 4505)
};

#endif // COROUTINE_THREAD_SWITCHING_H
