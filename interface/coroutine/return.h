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

#if __has_include(<coroutine/frame.h>)
#include <coroutine/frame.h>

namespace coro {
using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;

#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>

namespace coro {
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

#else
#error "requires header <experimental/coroutine> or <coroutine/frame.h>"
#endif // <experimental/coroutine>

/**
 * @defgroup Return
 * Types for easier coroutine promise/return type definition.
 */

/**
 * @brief   `suspend_never`(initial) + `suspend_never`(final)
 * @ingroup Return
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
 * @brief   `suspend_never`(initial) + `suspend_always`(final)
 * @ingroup Return
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
 * @brief   `suspend_always`(initial) + `suspend_never`(final)
 * @ingroup Return
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
 * @brief   `suspend_always`(initial) + `suspend_always`(final)
 * @ingroup Return
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
 * @brief   no suspend in initial/final + `void` return
 * @see     promise_nn
 * @ingroup Return
 */
class null_frame_promise : public promise_nn {
  public:
    void unhandled_exception() noexcept(false) {
        throw;
    }
    void return_void() noexcept {
    }
};

/**
 * @brief   `void` retrun for coroutine function
 * @note    The library supports `coroutine_traits` specialization for `nullptr_t`.
 *          This type is for GCC, which doesn't allow non-struct return.
 * 
 * @code
 * // for MSVC, Clang, GCC
 * auto coroutine_1() -> null_frame_t {
 *     co_await suspend_never{};
 * }
 * 
 * // GCC doesn't allow this! (`nullptr_t` is not class)
 * auto coroutine_2() -> nullptr_t {
 *     co_await suspend_never{};
 * }
 * @endcode
 */
struct null_frame_t final {
    struct promise_type : public null_frame_promise {
        /**
         * @brief Since this is template specialization for `void`, the return type is fixed to `void`
         */
        null_frame_t get_return_object() noexcept {
            return {};
        }
    };
};

/**
 * @brief   A type to acquire `coroutine_handle<void>` from anonymous coroutine's return. 
 *          Requires manual `destroy` of the coroutine handle.
 * 
 * @ingroup Return
 * @see coroutine_handle<void>
 * @see promise_na
 */
class frame_t : public coroutine_handle<void> {
  public:
    /**
     * @brief Acquire `coroutine_handle<void>` from current object and expose it through `get_return_object`
     */
    class promise_type : public promise_na {
      public:
        /**
         * @brief The `frame_t` will do nothing for exception handling
         */
        void unhandled_exception() noexcept(false) {
            throw;
        }
        void return_void() noexcept {
        }
        /**
         * @brief Acquire `coroutine_handle<void>` from current promise and return it
         * @return frame_t 
         */
        frame_t get_return_object() noexcept {
            return frame_t{coroutine_handle<promise_type>::from_promise(*this)};
        }
    };

  public:
    explicit frame_t(coroutine_handle<void> frame = nullptr) noexcept
        : coroutine_handle<void>{frame} {
    }
};

/**
 * @brief Suspend after invoke and expose its `coroutine_handle<void>` through return
 * @ingroup Return
 * @see coroutine_handle<void>
 * @see frame_t
 * @see promise_aa
 */
class passive_frame_t : public coroutine_handle<void> {
  public:
    class promise_type : public promise_aa {
      public:
        void unhandled_exception() noexcept(false) {
            throw;
        }
        void return_void() noexcept {
        }
        passive_frame_t get_return_object() noexcept {
            return passive_frame_t{
                coroutine_handle<promise_type>::from_promise(*this)};
        }
    };
    explicit passive_frame_t(coroutine_handle<void> frame = nullptr) noexcept
        : coroutine_handle<void>{frame} {
    }
};

#if defined(__cpp_concepts)
/*
template <typename T, typename R = void>
concept awaitable = requires(T a, coroutine_handle<void> h) {
    { a.await_ready() } ->bool;
    { a.await_suspend(h) } ->void;
    { a.await_resume() } ->R;
};
template <typename P>
concept promise_requirement_basic = requires(P p) {
    { p.initial_suspend() } ->awaitable;
    { p.final_suspend() } ->awaitable;
    { p.unhandled_exception() } ->void;
};
*/
#endif

} // namespace coro

namespace std {
namespace experimental {

/**
 * @brief Allow `void` return of the coroutine
 * 
 * @tparam P input parameter types of the coroutine's signature
 * @ingroup Return
 */
template <typename... P>
struct coroutine_traits<nullptr_t, P...> {
    struct promise_type : public coro::null_frame_promise {
        /**
         * @brief Since this is template specialization for `void`, the return type is fixed to `void`
         */
        nullptr_t get_return_object() noexcept {
            return nullptr;
        }
    };
};

} // namespace experimental
} // namespace std

#endif // COROUTINE_PROMISE_AND_RETURN_TYPES_H
