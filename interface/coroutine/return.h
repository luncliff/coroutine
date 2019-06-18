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

#include <coroutine/frame.h>
#include <exception>

namespace coro {
using namespace std::experimental;

// General `void` return for coroutine.
// It doesn't provide any method to get control or value from the resumable
// function
class no_return final {
  public:
    class promise_type final {
      public:
        // No suspend for init/final suspension point
        auto initial_suspend() noexcept {
            return suspend_never{};
        }
        auto final_suspend() noexcept {
            return suspend_never{};
        }
        void return_void() noexcept {
            // nothing to do because this is `void` return
        }
        void unhandled_exception() noexcept(false) {
            std::terminate();
        }
        auto get_return_object() noexcept -> promise_type* {
            return this;
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> promise_type* {
            return nullptr;
        }
        // void* operator new(size_t sz) noexcept {
        //     return malloc(sz); // for tracing frame life cycle ...
        // }
        // void operator delete(void* ptr, size_t sz) noexcept {
        //     free(ptr); // for tracing frame life cycle ...
        // }
    };

  public:
    no_return(const promise_type*) noexcept {
        // the type truncates all given info about its frame
    }
};

// Receiver type for coroutine's frame address
// This type has 2 use-case.
//  - the return type of coroutine function
//  - the operand of `co_await`. In the case, the coroutine will suspend
class frame final : public coroutine_handle<void> {
  public:
    struct promise_type final {
        auto initial_suspend() noexcept {
            return suspend_never{};
        }
        auto final_suspend() noexcept {
            return suspend_always{};
        }
        void return_void() noexcept {
            // nothing to do because this is `void` return
        }
        void unhandled_exception() noexcept(false) {
            std::terminate();
        }
        auto get_return_object() noexcept -> promise_type* {
            return this;
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> promise_type* {
            return nullptr;
        }
    };

  public:
    // override `suspend_always::await_suspend`
    // provide interface to receive handle after being used as an argument of
    // `co_await` by reference
    void await_suspend(coroutine_handle<void> coro) noexcept {
        coroutine_handle<void>& self = *this;
        self = coro;
    }
    constexpr bool await_ready() noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
    }

    frame() noexcept = default;
    frame(promise_type* p) noexcept {
        this->await_suspend(coroutine_handle<promise_type>::from_promise(*p));
    }
};
static_assert(sizeof(frame) == sizeof(coroutine_handle<void>));

} // namespace coro

#endif // COROUTINE_RETURN_TYPES_H