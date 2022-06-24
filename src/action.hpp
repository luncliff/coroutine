#pragma once
#include <cstdint>
#include <stdexcept>
#include <type_traits>

// ...
#include "coro.hpp"

namespace coro {

#if __has_include(<coroutine>) && (__cplusplus >= 202000L) // C++ 20
using std::coroutine_handle;
using std::noop_coroutine;
using std::suspend_always;
using std::suspend_never;

#elif __cplusplus >= 201700L // C++ 17
using std::experimental::coroutine_handle;
using std::experimental::noop_coroutine;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

#endif

void sink_exception(std::exception_ptr&& exp) noexcept(false);

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
        void unhandled_exception() noexcept {
            sink_exception(std::current_exception());
        }
        constexpr void return_void() noexcept {
        }
        fire_and_forget get_return_object() noexcept {
            return fire_and_forget{*this};
        }
    };

    explicit fire_and_forget([[maybe_unused]] promise_type&) noexcept {
    }
};

/**
 * @details `__builtin_coro_resume` fits the signature, but we can't get an address of it
 *  because it's a compiler intrinsic. Create a simple function for the purpose.
 * 
 * @see dispatch_function_t
 * @param ptr argument for `coroutine_handle<void>::from_address`
 */
void resume_once(void* ptr) noexcept;

struct end_awaitable_t final {
    coroutine_handle<void> handle;

  public:
    constexpr bool await_ready() noexcept {
        return false;
    }
    coroutine_handle<void> await_suspend([[maybe_unused]] coroutine_handle<void>) noexcept;
    constexpr void await_resume() noexcept {
    }
};

class paused_action_t final {
  public:
    class promise_type final {
        coroutine_handle<void> next = nullptr; // task that should be continued after `final_suspend`

      public:
        constexpr suspend_always initial_suspend() noexcept {
            return {};
        }
        end_awaitable_t final_suspend() noexcept {
            return {next ? next : noop_coroutine()};
        }
        void unhandled_exception() noexcept {
            sink_exception(std::current_exception());
        }
        constexpr void return_void() noexcept {
        }
        paused_action_t get_return_object() noexcept {
            return paused_action_t{*this};
        }

        void set_next(coroutine_handle<void> task) noexcept;
    };

    struct chained_awaitable_t final {
        coroutine_handle<promise_type> action; // handle of the `paused_action_t`

      public:
        constexpr bool await_ready() const noexcept {
            return false;
        }
        coroutine_handle<void> await_suspend(coroutine_handle<promise_type> coro) noexcept;
        constexpr void await_resume() const noexcept {
        }
    };

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

    chained_awaitable_t operator co_await() noexcept {
        // chain the next coroutine to given coroutine handle
        return {coro};
    }
};
static_assert(std::is_copy_constructible_v<paused_action_t> == false);
static_assert(std::is_copy_assignable_v<paused_action_t> == false);
static_assert(std::is_move_constructible_v<paused_action_t> == true);
static_assert(std::is_move_assignable_v<paused_action_t> == true);

struct event_proxy_t final {
    using fn_access_t = void (*)(void* c);
    using fn_signal_t = void (*)(void* c);
    using fn_wait_t = uint32_t (*)(void* c, uint32_t us);

  public:
    void* context;
    fn_access_t retain;  // e.retain(e.context);
    fn_access_t release; // e.release(e.context);
    fn_signal_t signal;  // e.signal(e.context);
    fn_wait_t wait;      // auto ec = e.wait(e.context, 0);
};

class waitable_action_t final {
  public:
    struct promise_type final {
        event_proxy_t proxy;

      public:
        suspend_never initial_suspend() noexcept;
        suspend_always final_suspend() noexcept;

        void return_void() noexcept;
        void unhandled_exception() noexcept {
            sink_exception(std::current_exception());
        }
        waitable_action_t get_return_object() noexcept {
            return waitable_action_t{this};
        }
    };

  private:
    promise_type* p;

  public:
    waitable_action_t(promise_type* p) noexcept;
    ~waitable_action_t() noexcept;

    void use(const event_proxy_t& e) noexcept(false);
    uint32_t wait(uint32_t us) noexcept(false);
};

} // namespace coro
