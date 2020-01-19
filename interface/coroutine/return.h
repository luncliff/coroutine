/**
 * @file coroutine/return.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief Utility to define return types for coroutine
 * @copyright CC BY 4.0
 * @link
 * https://devblogs.microsoft.com/cppblog/c20-concepts-are-here-in-visual-studio-2019-version-16-3/
 */
#pragma once
#ifndef COROUTINE_PROMISE_AND_RETURN_TYPES_H
#define COROUTINE_PROMISE_AND_RETURN_TYPES_H
#include <type_traits>

#include <coroutine/frame.h>

namespace coro {
#if __has_include(<coroutine>) // C++ 20
using namespace std;
#elif __has_include(<experimental/coroutine>) // C++ 17
using namespace std;
using namespace std::experimental;
#endif

class promise_nn {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{}; // no suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_never{}; // no suspend after return
    }
};

class promise_na {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{}; // no suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_always{}; // suspend after return
    }
};

class promise_an {
  public:
    auto initial_suspend() noexcept {
        return suspend_always{}; // suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_never{}; // no suspend after return
    }
};

class promise_aa {
  public:
    auto initial_suspend() noexcept {
        return suspend_always{}; // suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_always{}; // suspend after return
    }
};

/**
 * @brief A type to acquire `coroutine_handle<void>` from anonymous
 * coroutine's return
 * @see coroutine_handle<void>
 */
class frame_t final : public coroutine_handle<void> {
  public:
    class promise_type final : public coro::promise_na {
      public:
        /**
         * @brief The coroutine with the `frame_t` will do nothing for exception
         * handling
         */
        void unhandled_exception() noexcept(false) {
            throw;
        }
        /**
         * @brief this is a `void` return for the coroutines
         */
        void return_void() noexcept {
        }
        auto get_return_object() noexcept {
            frame_t frame{};
            coroutine_handle<void>& ref = frame;
            ref = coroutine_handle<promise_type>::from_promise(*this);
            return frame;
        }
    };
};

#if defined(__cpp_concepts)
template <typename T, typename R = void>
concept awaitable = requires(T a, coroutine_handle<void> h) {
    { a.await_ready() }
    ->bool;
    { a.await_suspend(h) }
    ->void;
    { a.await_resume() }
    ->R;
};

template <typename P>
concept promise_requirement_basic = requires(P p) {
    { p.initial_suspend() }
    ->awaitable;
    { p.final_suspend() }
    ->awaitable;
    { p.unhandled_exception() }
    ->void;
};

#endif

} // namespace coro

#endif // COROUTINE_PROMISE_AND_RETURN_TYPES_H
