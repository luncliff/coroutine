/**
 * @file coroutine/pthread.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 */
#ifndef COROUTINE_PTHREAD_UTILITY_H
#define COROUTINE_PTHREAD_UTILITY_H
#if not __has_include(<pthread.h>)
#error "expect <pthread.h> for this file"
#endif
#include <pthread.h>
#include <system_error>

#include <coroutine/return.h>

/**
 * @defgroup POSIX
 * Helpers to apply `co_await` for thread object/operations
 */

namespace coro {

/**
 * @brief Creates a new POSIX Thread and resume the given coroutine handle on it
 * @ingroup POSIX
 */
class pthread_spawner_t {
    /**
     * @see pthread_create
     * @param coro 
     */
    void resume_on_pthread(coroutine_handle<void> coro) noexcept(false);
    static void* pthread_resume(void* ptr) noexcept(false);

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        return resume_on_pthread(coro);
    }

  public:
    pthread_spawner_t(pthread_t* _tid, const pthread_attr_t* _attr)
        : tid{_tid}, attr{_attr} {
    }
    ~pthread_spawner_t() noexcept = default;

  private:
    pthread_t* const tid;
    const pthread_attr_t* const attr;
};

/**
 * @brief allows `pthread_attr_t*` for `co_await` operator 
 * @ingroup Thread
 * @see pthread_create
 * @see pthread_attr_t
 * @ingroup POSIX
 * 
 * The type wraps `pthread_create` function. 
 * After spawn, it contains thread id of the brand-new thread.
 */
class pthread_spawn_promise {
  public:
    pthread_t tid{};

  public:
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    /** @brief the activator is responsible for the exception handling */
    void unhandled_exception() noexcept(false) {
        throw;
    }

    /**
     * @brief co_await for `pthread_attr_t`
     */
    auto await_transform(const pthread_attr_t* attr) noexcept(false) {
        if (tid) // already created.
            throw std::logic_error{"pthread's spawn must be used once"};

        // provide the address at this point
        return pthread_spawner_t{&this->tid, attr};
    }
    inline auto await_transform(pthread_attr_t* attr) noexcept(false) {
        return await_transform(static_cast<const pthread_attr_t*>(attr));
    }

    /**
     * @brief general co_await
     */
    template <typename Awaitable>
    decltype(auto) await_transform(Awaitable&& a) noexcept {
        return std::forward<Awaitable&&>(a);
    }
};

/**
 * @brief A proxy to `pthread_spawn_promise`
 * @ingroup POSIX
 * 
 * The type must ensure the promise contains a valid `pthread_t` when it is constructed.
 */
class pthread_knower_t {
  public:
    /**
     * @brief allow access to the `tid`
     * @return pthread_t 
     */
    operator pthread_t() const noexcept {
        return promise->tid;
    }

  protected:
    explicit pthread_knower_t(pthread_spawn_promise* _promise) noexcept
        : promise{_promise} {
    }

  protected:
    pthread_spawn_promise* promise;
};

/**
 * @brief Special return type that wraps `pthread_join`
 * @ingroup POSIX
 * @see pthread_join
 */
class pthread_joiner_t final : public pthread_knower_t {
  public:
    class promise_type final : public pthread_spawn_promise {
      public:
        auto final_suspend() noexcept {
            return suspend_always{};
        }
        void return_void() noexcept {
            // we already returns coroutine's frame.
            // so `co_return` can't have its operand
        }
        auto get_return_object() noexcept -> promise_type* {
            return this;
        }
    };

  private:
    /**
     * @see pthread_join
     * @throw system_error
     */
    void try_join() noexcept(false);

  public:
    ~pthread_joiner_t() noexcept(false) {
        this->try_join();
    }
    /**
     * @throw invalid_argument
     */
    pthread_joiner_t(promise_type* p) noexcept(false);
};

/**
 * @brief Special return type that wraps `pthread_detach`
 * @ingroup POSIX
 */
class pthread_detacher_t final : public pthread_knower_t {
  public:
    class promise_type final : public pthread_spawn_promise {
      public:
        // detacher doesn't care about the coroutine frame's life cycle
        // it does nothing after `co_return`
        auto final_suspend() noexcept {
            return suspend_never{};
        }
        void return_void() noexcept {
        }
        auto get_return_object() noexcept -> promise_type* {
            return this;
        }
    };

  private:
    /**
     * @see pthread_detach
     * @throw system_error
     */
    void try_detach() noexcept(false);

  public:
    ~pthread_detacher_t() noexcept(false) {
        this->try_detach();
    }
    /**
     * @throw invalid_argument
     */
    pthread_detacher_t(promise_type* p) noexcept(false);
};

} // namespace coro

#endif // COROUTINE_PTHREAD_UTILITY_H
