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

} // namespace coro