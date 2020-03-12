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
#include <future>

namespace coro {
using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;

#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#include <future>

namespace coro {
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

#else
#error "requires header <experimental/coroutine> or <coroutine/frame.h>"
#endif

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
 * @brief A type to acquire `coroutine_handle<void>` from anonymous coroutine's return
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
            frame_t frame{};
            coroutine_handle<void>& ref = frame;
            ref = coroutine_handle<promise_type>::from_promise(*this);
            return frame;
        }
    };
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
            passive_frame_t frame{};
            coroutine_handle<void>& ref = frame;
            ref = coroutine_handle<promise_type>::from_promise(*this);
            return frame;
        }
    };
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
        nullptr_t get_return_object() noexcept {
            return nullptr;
        }
    };
};

#if !defined(_WIN32)
/**
 * @brief Support return of `std::future<T>` like VC++ did with `resumable_handle`
 * @ingroup Return
 * @see std::promise<T>
 */
template <typename R, typename... P>
struct coroutine_traits<future<R>, P...> {
    struct promise_type final : promise<R> {
        suspend_never initial_suspend() noexcept {
            return {};
        }
        suspend_never final_suspend() noexcept {
            return {};
        }
        void unhandled_exception() noexcept {
            this->set_exception(current_exception());
        }
        void return_value(R&& result) noexcept {
            this->set_value(move(result));
        }
        void return_value(R& result) noexcept {
            this->set_value(result);
        }
        auto get_return_object() noexcept -> future<R> {
            return this->get_future();
        }
    };
};
#endif
} // namespace experimental
} // namespace std

#endif // COROUTINE_PROMISE_AND_RETURN_TYPES_H
