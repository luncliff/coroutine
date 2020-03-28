/**
 * @file coroutine/pthread.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 */
#ifndef COROUTINE_PTHREAD_UTILITY_H
#define COROUTINE_PTHREAD_UTILITY_H
#if __has_include(<pthread.h>)
#include <pthread.h>
#else
#error "expect <pthread.h> for this file"
#endif
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
class continue_on_pthread final {
    /**
     * @see pthread_create
     * @param coro 
     */
    static uint32_t spawn(pthread_t& tid, const pthread_attr_t* attr,
                          coroutine_handle<void> coro) noexcept(false);
    static void* on_pthread(void* ptr) noexcept(false);

  private:
    pthread_t* const ptr;
    const pthread_attr_t* const attr;

  public:
    bool await_ready() const noexcept {
        return false;
    }
    void await_resume() noexcept {
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        if (int ec = spawn(*ptr, attr, coro))
            throw std::system_error{ec, std::system_category(),
                                    "pthread_create"};
    }

  public:
    continue_on_pthread(pthread_t& tid, const pthread_attr_t* attr)
        : ptr{&tid}, attr{attr} {
    }
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
    constexpr auto initial_suspend() noexcept {
        return suspend_never{};
    }
    constexpr void return_void() noexcept {
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
        return continue_on_pthread{tid, attr};
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
 * @brief Special return type that wraps `pthread_join`
 * @ingroup POSIX
 * @see pthread_join
 */
class pthread_joiner final {
    friend class promise_type;

  public:
    class promise_type final : public pthread_spawn_promise {
      public:
        constexpr auto final_suspend() noexcept {
            return suspend_always{};
        }
        auto get_return_object() noexcept {
            return pthread_joiner{this};
        }
    };

  private:
    pthread_spawn_promise* promise;

  private:
    explicit pthread_joiner(promise_type* p) noexcept(false);

  public:
    /**
     * @brief try to join the related thread
     * @see pthread_join
     * @throw system_error
     */
    ~pthread_joiner() noexcept(false);

    pthread_joiner(const pthread_joiner&) = delete;
    pthread_joiner& operator=(const pthread_joiner&) = delete;
    pthread_joiner(pthread_joiner&&) = default;
    pthread_joiner& operator=(pthread_joiner&&) = default;

    /**
     * @brief allow access to the `tid`
     * @return pthread_t 
     */
    operator pthread_t() const noexcept {
        return promise->tid;
    }
};

/**
 * @brief Special return type that wraps `pthread_detach`
 * @ingroup POSIX
 */
class pthread_detacher final {
    friend class promise_type;

  public:
    class promise_type final : public pthread_spawn_promise {
      public:
        // detacher doesn't care about the coroutine frame's life cycle
        // it does nothing after `co_return`
        constexpr auto final_suspend() noexcept {
            return suspend_never{};
        }
        auto get_return_object() noexcept {
            return pthread_detacher{this};
        }
    };

  private:
    pthread_spawn_promise* promise;

  private:
    explicit pthread_detacher(promise_type* p) noexcept(false);

  public:
    /**
     * @brief try to detach the related thread
     * @see pthread_detach
     * @throw system_error
     */
    ~pthread_detacher() noexcept(false);
    pthread_detacher(const pthread_detacher&) = delete;
    pthread_detacher& operator=(const pthread_detacher&) = delete;
    pthread_detacher(pthread_detacher&&) = default;
    pthread_detacher& operator=(pthread_detacher&&) = default;

    /**
     * @brief allow access to the `tid`
     * @return pthread_t 
     */
    operator pthread_t() const noexcept {
        return promise->tid;
    }
};

} // namespace coro

#endif // COROUTINE_PTHREAD_UTILITY_H
