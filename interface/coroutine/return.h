/**
 * @file coroutine/return.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief Utility to define return types for coroutine
 * @copyright CC BY 4.0
 * @link https://devblogs.microsoft.com/cppblog/c20-concepts-are-here-in-visual-studio-2019-version-16-3/
 */
#pragma once
#ifndef COROUTINE_PROMISE_AND_RETURN_TYPES_H
#define COROUTINE_PROMISE_AND_RETURN_TYPES_H
#include <type_traits>

#include <coroutine/frame.h>

/**
 * @defgroup Return
 * Types for easier coroutine promise/return type definition.
 */

namespace coro {
#if __has_include(<coroutine>) // C++ 20
using namespace std;
#elif __has_include(<experimental/coroutine>) // C++ 17
using namespace std;
using namespace std::experimental;
#endif

/**
 * @ingroup Return
 * @note `suspend_never`(initial) + `suspend_never`(final)
 */
class promise_nn {
  public:
    /**
     * @brief no suspend after invoke
     * @return suspend_never 
     */
    suspend_never initial_suspend() noexcept {
        return {};
    }
    /**
     * @brief destroy coroutine frame after return
     * @return suspend_never 
     */
    suspend_never final_suspend() noexcept {
        return {};
    }
};

/**
 * @ingroup Return
 * @note `suspend_never`(initial) + `suspend_always`(final)
 */
class promise_na {
  public:
    /**
     * @brief no suspend after invoke
     * @return suspend_never 
     */
    suspend_never initial_suspend() noexcept {
        return {};
    }
    /**
     * @brief suspend after return
     * @return suspend_always 
     */
    suspend_always final_suspend() noexcept {
        return {};
    }
};

/**
 * @ingroup Return
 * @note `suspend_always`(initial) + `suspend_never`(final)
 */
class promise_an {
  public:
    /**
     * @brief suspend after invoke
     * @return suspend_always 
     */
    suspend_always initial_suspend() noexcept {
        return {};
    }
    /**
     * @brief destroy coroutine frame after return
     * @return suspend_never 
     */
    suspend_never final_suspend() noexcept {
        return {};
    }
};

/**
 * @ingroup Return
 * @note `suspend_always`(initial) + `suspend_always`(final)
 */
class promise_aa {
  public:
    /**
     * @brief suspend after invoke
     * @return suspend_always 
     */
    suspend_always initial_suspend() noexcept {
        return {};
    }
    /**
     * @brief suspend after return
     * @return suspend_always 
     */
    suspend_always final_suspend() noexcept {
        return {};
    }
};

/**
 * @brief A type to acquire `coroutine_handle<void>` from anonymous coroutine's return
 * @ingroup Return
 * @see coroutine_handle<void>
 */
class frame_t final : public coroutine_handle<void> {
  public:
    /**
     * @brief Acquire `coroutine_handle<void>` from current object and expose it through `get_return_object`
     */
    class promise_type final : public promise_na {
      public:
        /**
         * @brief The `frame_t` will do nothing for exception handling
         */
        void unhandled_exception() noexcept(false) {
            throw;
        }
        /**
         * @brief this is a `void` return for the coroutines
         */
        void return_void() noexcept {
        }
        /**
         * @brief Acquire `coroutine_handle<void>` from current promise and return it
         * @return frame_t 
         */
        frame_t get_return_object() noexcept {
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

namespace std {
namespace experimental {

/**
 * @brief Allow `void` return of the coroutine
 * 
 * @tparam P input parameter types of the coroutine's signature
 */
template <typename... P>
struct coroutine_traits<void, P...> {
    struct promise_type final {
        suspend_never initial_suspend() noexcept {
            return {};
        }
        suspend_never final_suspend() noexcept {
            return {};
        }
        void unhandled_exception() noexcept(false) {
            throw;
        }
        /**
         * @brief Since this is template specialization for `void`, the return type is fixed to `void`
         */
        void return_void() noexcept {
        }
        /**
         * @brief Since this is template specialization for `void`, the return type is fixed to `void`
         */
        void get_return_object() noexcept {
        }
    };
};

} // namespace experimental

template <typename Ret, typename... Param>
using coroutine_traits = std::experimental::coroutine_traits<Ret, Param...>;

} // namespace std

#endif // COROUTINE_PROMISE_AND_RETURN_TYPES_H
