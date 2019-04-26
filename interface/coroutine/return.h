// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Return and utility types for coroutine frame management
//
// ---------------------------------------------------------------------------
#pragma once
#ifndef COROUTINE_RETURN_TYPES_H
#define COROUTINE_RETURN_TYPES_H

#include <exception>

#include <coroutine/frame.h>

namespace coro
{
using namespace std::experimental;

// - Note
//      General `void` return for coroutine.
//      It doesn't provide any method to get control or value
//       from the resumable function
class return_ignore final
{
  public:
    class promise_type final
    {
      public:
        // No suspend for init/final suspension point
        auto initial_suspend() noexcept
        {
            return suspend_never{};
        }
        auto final_suspend() noexcept
        {
            return suspend_never{};
        }
        void return_void() noexcept
        {
            // nothing to do because this is `void` return
        }
        void unhandled_exception() noexcept(false)
        {
            std::terminate(); // customize this part
        }

        promise_type* get_return_object() noexcept
        {
            return this;
        }
        static promise_type* get_return_object_on_allocation_failure() noexcept
        {
            return nullptr;
        }
    };

  public:
    return_ignore(const promise_type*) noexcept
    {
        // the type truncates all given info about its frame
    }
};

// - Note
//      Holds the resumable function's frame
//      This type can be used when final suspend is required
class return_frame final
{
  public:
    class promise_type;

  private:
    coroutine_handle<void> frame{};

  public:
    return_frame() noexcept = default;
    return_frame(promise_type* ptr) noexcept : return_frame{}
    {
        frame = coroutine_handle<promise_type>::from_promise(*ptr);
    }

  public:
    auto get() const noexcept
    {
        return frame;
    }
    explicit operator coroutine_handle<void>() const noexcept
    {
        return frame;
    }

  public:
    class promise_type final
    {
      public:
        auto initial_suspend() noexcept
        {
            return suspend_never{};
        }
        auto final_suspend() noexcept
        {
            // !!! notice this behavior !!!
            return suspend_always{};
        }
        void return_void() noexcept
        {
            // nothing to do because this is `void` return
        }
        void unhandled_exception() noexcept(false)
        {
            // user can customize this point with std::current_exception ...
            // by default, terminate the program.
            std::terminate();
        }

        promise_type* get_return_object() noexcept
        {
            return this;
        }
    };
};

} // namespace coro

#endif // COROUTINE_RETURN_TYPES_H
