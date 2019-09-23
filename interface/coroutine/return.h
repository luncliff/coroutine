//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Return and utility types for coroutine frame management
//
#pragma once
#ifndef COROUTINE_PROMISE_AND_RETURN_TYPES_H
#define COROUTINE_PROMISE_AND_RETURN_TYPES_H

#if __has_include(<coroutine/frame.h>)
#include <coroutine/frame.h>
#elif __has_include(<experimental/coroutine>) // C++ 17
#include <experimental/coroutine>
#elif __has_include(<coroutine>) // C++ 20
#include <coroutine>
#else
#error "expect header <experimental/coroutine> or <coroutine/frame.h>"
#endif

namespace coro {
using namespace std::experimental;

class promise_return_destroy {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{}; // no suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_never{}; // no suspend after return
    }
    void unhandled_exception() noexcept(false) {
        std::terminate();
    }
};

class promise_return_preserve {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{}; // no suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_always{}; // suspend after return
    }
    void unhandled_exception() noexcept(false) {
        std::terminate();
    }
};

class promise_manual_control {
  public:
    auto initial_suspend() noexcept {
        return suspend_always{}; // suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_always{}; // suspend after return
    }
    void unhandled_exception() noexcept(false) {
        std::terminate();
    }
};

class forget_frame {
  public:
    class promise_type final : public promise_return_destroy {
      public:
        void return_void() noexcept {
            // nothing to do because this is `void` return
        }
        auto get_return_object() noexcept -> forget_frame {
            return {this};
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> forget_frame {
            return {nullptr};
        }
    };

  private:
    forget_frame(const promise_type*) noexcept {
        // the type truncates all given info about its frame
    }

  public:
    // gcc-10 requires the type to be default constructible
    forget_frame() noexcept = default;
};

class preserve_frame : public coroutine_handle<void> {
  public:
    class promise_type final : public promise_return_preserve {
      public:
        void return_void() noexcept {
            // nothing to do because this is `void` return
        }
        auto get_return_object() noexcept -> preserve_frame {
            return {this};
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> preserve_frame {
            return {nullptr};
        }
    };

  private:
    preserve_frame(promise_type* p) noexcept : coroutine_handle<void>{} {
        if (p == nullptr)
            return;
        coroutine_handle<void>& ref = *this;
        ref = coroutine_handle<promise_type>::from_promise(*p);
    }

  public:
    // gcc-10 requires the type to be default constructible
    preserve_frame() noexcept = default;
};

class save_frame_t final {
  public:
    // provide interface to receive handle
    // when it's used as an operand of `co_await`
    void await_suspend(coroutine_handle<void> coro) noexcept {
        ref = coro;
    }
    constexpr bool await_ready() noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
    }

  public:
    explicit save_frame_t(coroutine_handle<void>& target) : ref{target} {
    }

  private:
    coroutine_handle<void>& ref;
};

} // namespace coro

#endif // COROUTINE_PROMISE_AND_RETURN_TYPES_H
