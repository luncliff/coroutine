// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once
#ifndef COROUTINE_RETURN_TYPES_H
#define COROUTINE_RETURN_TYPES_H

#include <coroutine/frame.h>
#include <stdexcept>

// - Note
//      General `void` return for coroutine.
//      It doesn't provide any method to get flow/value
//       from the resumable function frame.
//
//      This type is an alternative of
//       the `std::future<void>` without final suspend
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

        // void* operator new(size_t sz) noexcept(false)
        // {
        //     auto ptr = calloc(sz, sizeof(char));
        //     printf("new(unplug): %p %zu \n", ptr, sz);
        //     return ptr;
        // }

        // void operator delete(void* ptr, size_t sz) noexcept
        // {
        //     printf("delete(unplug): %p %zu \n", ptr, sz);
        //     free(ptr);
        // }
    };

  public:
    unplug(const promise_type*) noexcept
    {
    }
};

// - Note
//      Holds the resumable function's frame
//      This type can be used when final suspend is required
//
//      This type is an alternative of
//       the `std::future<void>` with final suspend
class frame_holder final
{
  public:
    struct promise_type;

    template <typename P>
    using coroutine_handle = std::experimental::coroutine_handle<P>;

  private:
    coroutine_handle<void> frame;

  private:
    frame_holder(const frame_holder&) = delete;
    frame_holder& operator=(const frame_holder&) = delete;

  public:
    frame_holder() noexcept : frame{}
    {
    }
    frame_holder(promise_type* ptr) noexcept
        : frame{coroutine_handle<promise_type>::from_promise(*ptr)}
    {
    }
    frame_holder(frame_holder&& rhs) noexcept : frame{nullptr}
    {
        std::swap(this->frame, rhs.frame);
    }
    frame_holder& operator=(frame_holder&& rhs) noexcept
    {
        std::swap(this->frame, rhs.frame);
        return *this;
    }
    ~frame_holder() noexcept
    {
        if (frame)
            frame.destroy();
    }

  public:
    struct promise_type final
    {
      public:
        // No suspend for init/final suspension point
        auto initial_suspend() noexcept
        {
            return std::experimental::suspend_never{};
        }
        auto final_suspend() noexcept
        {
            return std::experimental::suspend_always{};
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
        }

        promise_type* get_return_object() noexcept
        {
            return this;
        }

        // void* operator new(size_t sz) noexcept(false)
        // {
        //     auto ptr = calloc(sz, sizeof(char));
        //     printf("new(frame_holder): %p %zu \n", ptr, sz);
        //     return ptr;
        // }

        // void operator delete(void* ptr, size_t sz) noexcept
        // {
        //     printf("delete(frame_holder): %p %zu \n", ptr, sz);
        //     free(ptr);
        // }
    };
};

// - Note
//      Receiver for explicit `co_await` to enable manual resume
class suspend_hook final
{
    std::experimental::coroutine_handle<void> frame{};

  public:
    bool await_ready() const noexcept
    {
        return false; // suspend_always
    }
    void await_suspend(std::experimental::coroutine_handle<void> coro) noexcept
    {
        // coroutine frame's prefix
        frame = std::move(coro);
    }
    void await_resume() noexcept
    {
        frame = nullptr; // forget
    }

  public:
    void resume() noexcept(false)
    {
        // resume if available
        if (frame && frame.done() == false)
            frame.resume();
    }
};

#endif // COROUTINE_RETURN_TYPES_H
