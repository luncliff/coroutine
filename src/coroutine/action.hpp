/**
 * @see https://gist.github.com/luncliff/1fedae034c001a460e9233ecf0afc25b
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#pragma once
#include <stdexcept>
#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#endif

/// @todo remove this import
#if __has_include(<dispatch/dispatch.h>)
#include <dispatch/dispatch.h>
#if __has_include(<dispatch/private.h>)
#include <dispatch/private.h>
#endif
#endif

namespace coro {
#if __has_include(<coroutine>)
using std::coroutine_handle;
using std::noop_coroutine;
using std::suspend_always;
using std::suspend_never;

#elif __has_include(<experimental/coroutine>)
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;
#if !defined(_WIN32)
using std::experimental::noop_coroutine;
#endif

#endif

/**
 * @see C++/WinRT `winrt::fire_and_forget`
 */
struct fire_and_forget final {
    struct promise_type final {
        constexpr suspend_never initial_suspend() noexcept {
            return {};
        }
        constexpr suspend_never final_suspend() noexcept {
            return {};
        }
        void unhandled_exception() noexcept;

        constexpr void return_void() noexcept {
        }
        fire_and_forget get_return_object() noexcept {
            return fire_and_forget{*this};
        }
    };

    explicit fire_and_forget([[maybe_unused]] promise_type&) noexcept {
    }
};

class paused_action_t;

class paused_promise_t final {
    coroutine_handle<void> next = nullptr; // task that should be continued after `final_suspend`

  public:
    constexpr suspend_always initial_suspend() noexcept {
        return {};
    }
    auto final_suspend() noexcept {
        struct awaitable_t final {
            coroutine_handle<void> handle;

          public:
            constexpr bool await_ready() noexcept {
                return false;
            }
            constexpr coroutine_handle<void> await_suspend(coroutine_handle<void>) noexcept {
                return handle;
            }
            constexpr void await_resume() noexcept {
            }
        };
        return awaitable_t{next ? next : noop_coroutine()};
    }
    void unhandled_exception() noexcept;

    constexpr void return_void() noexcept {
    }
    paused_action_t get_return_object() noexcept;

    void set_next(coroutine_handle<void> task) noexcept;
};

class paused_action_t final {
  public:
    using promise_type = paused_promise_t;

  private:
    coroutine_handle<promise_type> coro;

  public:
    explicit paused_action_t(promise_type& p) noexcept;
    ~paused_action_t() noexcept;
    paused_action_t(const paused_action_t&) = delete;
    paused_action_t& operator=(const paused_action_t&) = delete;
    paused_action_t(paused_action_t&& rhs) noexcept;
    paused_action_t& operator=(paused_action_t&& rhs) noexcept;

    coroutine_handle<void> handle() const noexcept;

    auto operator co_await() noexcept {
        struct awaitable_t final {
            coroutine_handle<promise_type> action; // handle of the `paused_action_t`

          public:
            constexpr bool await_ready() const noexcept {
                return false;
            }
            coroutine_handle<void> await_suspend(coroutine_handle<promise_type> coro) noexcept {
                action.promise().set_next(coro); // save the task so it can be continued
                return action;
            }
            constexpr void await_resume() const noexcept {
            }
        };
        // chain the next coroutine to given coroutine handle
        return awaitable_t{coro};
    }
};
static_assert(std::is_copy_constructible_v<paused_action_t> == false);
static_assert(std::is_copy_assignable_v<paused_action_t> == false);
static_assert(std::is_move_constructible_v<paused_action_t> == true);
static_assert(std::is_move_assignable_v<paused_action_t> == true);

/**
 * @details `__builtin_coro_resume` fits the signature, but we can't get an address of it
 *  because it's a compiler intrinsic. Create a simple function for the purpose.
 * 
 * @see dispatch_function_t
 * @param ptr argument for `coroutine_handle<void>::from_address`
 */
void resume_once(void* ptr) noexcept;

#if __has_include(<dispatch/dispatch.h>)

/**
 * @brief Forward the `coroutine_handle`(job) to `dispatch_queue_t`
 * @see dispatch_async_f
 * @see https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/OperationQueues/OperationQueues.html
 */
struct queue_awaitable_t final {
    dispatch_queue_t queue;

  public:
    /// @brief true if `queue` is nullptr, resume immediately
    constexpr bool await_ready() const noexcept {
        return queue == nullptr;
    }
    /// @see dispatch_async_f
    void await_suspend(coroutine_handle<void> coro) noexcept;

    constexpr void await_resume() const noexcept {
    }
};
static_assert(std::is_nothrow_copy_constructible_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_copy_assignable_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_move_constructible_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_move_assignable_v<queue_awaitable_t> == true);

class semaphore_owner_t final {
    dispatch_semaphore_t sem;

  public:
    semaphore_owner_t() noexcept(false);
    explicit semaphore_owner_t(dispatch_semaphore_t handle) noexcept(false);
    ~semaphore_owner_t() noexcept;
    semaphore_owner_t(const semaphore_owner_t& rhs) noexcept;
    semaphore_owner_t(semaphore_owner_t&&) = delete;
    semaphore_owner_t& operator=(const semaphore_owner_t&) = delete;
    semaphore_owner_t& operator=(semaphore_owner_t&&) = delete;

    /// @todo provide a return value?
    bool signal() noexcept {
        return dispatch_semaphore_signal(sem);
    }
    [[nodiscard]] bool wait() noexcept {
        return dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0;
    }
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds duration) noexcept {
        return dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, duration.count())) == 0;
    }

    dispatch_semaphore_t handle() const noexcept {
        return sem;
    }
};
static_assert(std::is_copy_constructible_v<semaphore_owner_t> == true);
static_assert(std::is_copy_assignable_v<semaphore_owner_t> == false);
static_assert(std::is_move_constructible_v<semaphore_owner_t> == false);
static_assert(std::is_move_assignable_v<semaphore_owner_t> == false);

/**
 * @brief A coroutine return type which supports wait/wait_for with `dispatch_semaphore_t`
 * @details When this type is used, the coroutine's first argument must be a valid `dispatch_semaphore_t`
 */
class semaphore_action_t final {
  public:
    class promise_type final {
        friend class semaphore_action_t;

        dispatch_semaphore_t semaphore;

      public:
        /// @note This signature is for Clang compiler. defined(__clang__)
        template <typename... Args>
        promise_type(dispatch_semaphore_t handle, [[maybe_unused]] Args&&...) noexcept(false) : semaphore{handle} {
        }
        /// @note This signature is for MSVC. defined(_MSC_VER)
        promise_type() noexcept = default;
        promise_type(const promise_type&) = delete;
        promise_type(promise_type&&) = delete;
        promise_type& operator=(const promise_type&) = delete;
        promise_type& operator=(promise_type&&) = delete;

        suspend_never initial_suspend() noexcept {
            dispatch_retain(semaphore);
            return {};
        }
        suspend_never final_suspend() noexcept {
            dispatch_release(semaphore);
            return {};
        }
        void unhandled_exception() noexcept;

        void return_void() noexcept {
            dispatch_semaphore_signal(semaphore);
        }
        semaphore_action_t get_return_object() noexcept {
            return semaphore_action_t{*this};
        }
    };

  private:
    semaphore_owner_t sem;

  private:
    /// @note change the `promise_type`'s handle before `initial_suspend`
    explicit semaphore_action_t(promise_type& p) noexcept;

  public:
    semaphore_action_t(const semaphore_action_t&) noexcept = default;
    semaphore_action_t& operator=(const semaphore_action_t&) noexcept = delete;
    semaphore_action_t& operator=(semaphore_action_t&&) noexcept = delete;

    [[nodiscard]] bool wait() noexcept {
        return sem.wait();
    }
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds duration) noexcept {
        return sem.wait_for(duration);
    }
};
static_assert(std::is_copy_constructible_v<semaphore_action_t> == true);
static_assert(std::is_copy_assignable_v<semaphore_action_t> == false);
// static_assert(std::is_move_constructible_v<semaphore_action_t> == false);
static_assert(std::is_move_assignable_v<semaphore_action_t> == false);

/**
* @see dispatch_group_notify_f
*/
struct group_awaitable_t final {
    dispatch_group_t group;
    dispatch_queue_t queue;

  public:
    [[nodiscard]] bool await_ready() const noexcept {
        return dispatch_group_wait(group, DISPATCH_TIME_NOW) == 0;
    }
    /// @see dispatch_group_notify_f
    void await_suspend(coroutine_handle<void> coro) const noexcept {
        return dispatch_group_notify_f(group, queue, coro.address(), resume_once);
    }
    constexpr dispatch_group_t await_resume() const noexcept {
        return group;
    }
};

class group_action_t final {
  public:
    class promise_type final {
        dispatch_group_t group;

      public:
        // explicit promise_type(dispatch_group_t group) noexcept(false) : group{group} {
        //     if (group == nullptr)
        //         throw std::invalid_argument{__func__};
        //     dispatch_retain(group);
        // }
        template <typename... Args>
        promise_type(dispatch_group_t group, [[maybe_unused]] Args&&...) noexcept(false) : group{group} {
            if (group == nullptr)
                throw std::invalid_argument{__func__};
            dispatch_retain(group);
        }
        ~promise_type() {
            dispatch_release(group);
        }
        promise_type(const promise_type&) = delete;
        promise_type(promise_type&&) = delete;
        promise_type& operator=(const promise_type&) = delete;
        promise_type& operator=(promise_type&&) = delete;

        suspend_always initial_suspend() noexcept {
            dispatch_group_enter(group);
            return {};
        }
        suspend_never final_suspend() noexcept {
            dispatch_group_leave(group);
            return {};
        }
        void unhandled_exception() noexcept;

        constexpr void return_void() noexcept {
        }
        group_action_t get_return_object() noexcept {
            return group_action_t{*this, group};
        }
    };

  private:
    coroutine_handle<promise_type> coro;
    dispatch_group_t const group;

  public:
    /**
    * @note `dispatch_group_async_f` retains the group. so this type won't modify reference count of it
    * @see run_on
    */
    group_action_t(promise_type& p, dispatch_group_t group) noexcept
        : coro{coroutine_handle<promise_type>::from_promise(p)}, group{group} {
    }
    group_action_t(const group_action_t&) = delete;
    group_action_t(group_action_t&&) noexcept = default;
    group_action_t& operator=(const group_action_t&) = delete;
    group_action_t& operator=(group_action_t&&) = delete;

    /**
    * @brief Schedule the initial-suspended coroutine to the given queue
    * @note Using this function more than 1 will cause undefined behavior
    * @see dispatch_group_async_f
    * @param queue destination queue to schedule current coroutine handle
    */
    void run_on(dispatch_queue_t queue) noexcept {
        dispatch_group_async_f(group, queue, coro.address(), resume_once);
        coro = nullptr; // suspend_never in `promise_type::final_suspend`
    }
};
static_assert(std::is_copy_constructible_v<group_action_t> == false);
static_assert(std::is_copy_assignable_v<group_action_t> == false);
static_assert(std::is_move_constructible_v<group_action_t> == true);
static_assert(std::is_move_assignable_v<group_action_t> == false);

/**
* @brief Hold a timer and expose some shortcuts to start/suspend/cancel
* @see dispatch_source_create
* @see DISPATCH_SOURCE_TYPE_TIMER
*/
class timer_owner_t final {
    dispatch_source_t source;

  public:
    explicit timer_owner_t(dispatch_source_t timer) noexcept(false) : source{timer} {
        if (source == nullptr)
            throw std::runtime_error{"dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER)"};
        dispatch_retain(source);
    }
    explicit timer_owner_t(dispatch_queue_t queue) noexcept(false)
        : timer_owner_t{dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue)} {
    }
    ~timer_owner_t() noexcept {
        dispatch_release(source);
    }
    timer_owner_t(const timer_owner_t&) = delete;
    timer_owner_t& operator=(const timer_owner_t&) = delete;
    timer_owner_t(timer_owner_t&&) = delete;
    timer_owner_t& operator=(timer_owner_t&&) = delete;

    dispatch_source_t handle() const noexcept {
        return source;
    }
    void set(void* context, dispatch_function_t on_event, dispatch_function_t on_cancel) noexcept;
    void start(std::chrono::nanoseconds interval, dispatch_time_t start) noexcept;

    /**
    * @param context   context of dispatch_source_t to change
    * @param on_cancel cancel handler of dispatch_source to change
    * @return true if the timer is canceled
    */
    bool cancel(void* context = nullptr, dispatch_function_t on_cancel = nullptr) noexcept;

    void suspend() noexcept;
};

/**
* @brief Similar to `winrt::resume_after`, but works with `dispatch_queue_t`
* @see C++/WinRT `winrt::resume_after`
*/
struct resume_after_t final {
    dispatch_queue_t queue;
    std::chrono::nanoseconds duration;

  public:
    /// @brief true if `duration` is 0
    constexpr bool await_ready() const noexcept {
        return duration.count() == 0;
    }
    /// @see dispatch_after_f
    void await_suspend(coroutine_handle<void> coro) noexcept {
        const dispatch_time_t timepoint = dispatch_time(DISPATCH_TIME_NOW, duration.count());
        dispatch_after_f(timepoint, queue, coro.address(), resume_once);
    }
    constexpr void await_resume() const noexcept {
    }
    resume_after_t& operator=(std::chrono::nanoseconds duration) noexcept {
        this->duration = duration;
        return *this;
    }
};
static_assert(std::is_nothrow_copy_constructible_v<resume_after_t> == true);
static_assert(std::is_nothrow_copy_assignable_v<resume_after_t> == true);
static_assert(std::is_nothrow_move_constructible_v<resume_after_t> == true);
static_assert(std::is_nothrow_move_assignable_v<resume_after_t> == true);

class queue_owner_t {
    dispatch_queue_t queue = nullptr;

  public:
    queue_owner_t(const char* label, dispatch_queue_attr_t attr) : queue{dispatch_queue_create(label, attr)} {
    }
    queue_owner_t(const char* label, dispatch_queue_attr_t attr, void* context, dispatch_function_t on_finalize)
        : queue_owner_t{label, attr} {
        dispatch_set_context(queue, context);
        dispatch_set_finalizer_f(queue, on_finalize);
    }
    ~queue_owner_t() {
        dispatch_release(queue);
    }
    queue_owner_t(const queue_owner_t&) = delete;
    queue_owner_t(queue_owner_t&&) = delete;
    queue_owner_t& operator=(const queue_owner_t&) = delete;
    queue_owner_t& operator=(queue_owner_t&&) = delete;

    dispatch_queue_t handle() const noexcept {
        return queue;
    }
};
static_assert(std::is_copy_constructible_v<queue_owner_t> == false);
static_assert(std::is_copy_assignable_v<queue_owner_t> == false);
static_assert(std::is_move_constructible_v<queue_owner_t> == false);
static_assert(std::is_move_assignable_v<queue_owner_t> == false);

#endif

} // namespace coro
