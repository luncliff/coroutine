//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Return and utility types for coroutine frame management
//
#pragma once
#ifndef LUNCLIFF_COROUTINE_RETURN_TYPES_H
#define LUNCLIFF_COROUTINE_RETURN_TYPES_H

#include <coroutine/frame.h>

namespace coro {
using namespace std::experimental;

class on_return_destroy {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    auto final_suspend() noexcept {
        return suspend_never{};
    }
    void unhandled_exception() noexcept(false) {
        // customize this part
    }
};

class on_return_preserve {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    auto final_suspend() noexcept {
        return suspend_always{};
    }
    void unhandled_exception() noexcept(false) {
        // customize this part
    }
};

class no_return final {
  public:
    class promise_type final : public on_return_destroy {
      public:
        void return_void() noexcept {
            // nothing to do because this is `void` return
        }
        auto get_return_object() noexcept -> no_return {
            return {this};
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> no_return {
            return {nullptr};
        }
    };

  private:
    no_return(const promise_type*) noexcept {
        // the type truncates all given info about its frame
    }

  public:
    // gcc-10 requires the type to be default constructible
    no_return() noexcept = default;
};

class preserve_frame final {
  public:
    class promise_type final : public on_return_preserve {
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
    preserve_frame(const promise_type*) noexcept {
        // the type truncates all given info about its frame
    }

  public:
    // gcc-10 requires the type to be default constructible
    preserve_frame() noexcept = default;
};

class save_frame_to final {
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
    explicit save_frame_to(coroutine_handle<void>& target) : ref{target} {
    }

  private:
    coroutine_handle<void>& ref;
};

} // namespace coro

#endif // LUNCLIFF_COROUTINE_RETURN_TYPES_H