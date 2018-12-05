// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once
#ifndef COROUTINE_UNPLUG_HPP
#define COROUTINE_UNPLUG_HPP

#include <coroutine/frame.h>

// - Note
//      this line is example for memory management customization.
// static std::allocator<char> __mgmt{};

// - Note
//      General `void` return for coroutine.
//      It doesn't provide any method to get flow/value
//      from the resumable function frame.
//
//      This type is an alternative of the `std::future<void>`
class unplug final
{
  public:
    struct promise_type
    {
      public:
        // No suspend for init/final suspension point
        auto initial_suspend() noexcept
        {
            return std::experimental::suspend_never{};
        }
        auto final_suspend() noexcept
        {
            return std::experimental::suspend_never{};
        }
        void return_void(void) noexcept
        {
            // nothing to do because this is `void` return
        }
        void unhandled_exception() noexcept(false)
        {
            // throw again
            if (auto eptr = std::current_exception())
                std::rethrow_exception(eptr);

            // terminate the program.
            std::terminate();
        }

        promise_type* get_return_object() noexcept
        {
            return this;
        }

        // - Note
        //      Examples for memory management customization.
        // void* operator new(size_t _size) noexcept(false)
        // {
        //     return __mgmt.allocate(_size);
        // }

        // - Note
        //      Examples for memory management customization.
        // void operator delete(void* _ptr, size_t _size) noexcept
        // {
        //     return __mgmt.deallocate(static_cast<char*>(_ptr), _size);
        // }
    };

  public:
    unplug(const promise_type*) noexcept
    {
    }
};

// - Note
//      Receiver for explicit `co_await` to enable manual resume
class await_point final
{
    void* prefix{};

  public:
    bool await_ready() const noexcept
    {
        return false; // suspend_always
    }
    void await_suspend(std::experimental::coroutine_handle<void> rh) noexcept
    {
        // coroutine frame's prefix
        prefix = rh.address();
    }
    void await_resume() noexcept
    {
        prefix = nullptr; // forget
    }

  public:
    void resume() noexcept(false)
    {
        if (auto coro
            = std::experimental::coroutine_handle<void>::from_address(prefix))
            if (coro.done() == false) // resume if available
                coro.resume();
    }
};

#endif // COROUTINE_UNPLUG_HPP
