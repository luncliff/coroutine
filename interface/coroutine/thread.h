//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
// clang-format off
#if defined(FORCE_STATIC_LINK)
#   define _INTERFACE_
#   define _HIDDEN_
#elif defined(_MSC_VER) // MSVC or clang-cl
#   define _HIDDEN_
#   ifdef _WINDLL
#       define _INTERFACE_ __declspec(dllexport)
#   else
#       define _INTERFACE_ __declspec(dllimport)
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define _INTERFACE_ __attribute__((visibility("default")))
#   define _HIDDEN_ __attribute__((visibility("hidden")))
#else
#   error "unexpected linking configuration"
#endif
// clang-format on

#ifndef COROUTINE_THREAD_UTILITY_H
#define COROUTINE_THREAD_UTILITY_H

#include <system_error>

#if __has_include(<coroutine/frame.h>)
#include <coroutine/frame.h>
#elif __has_include(<experimental/coroutine>) // C++ 17
#include <experimental/coroutine>
#elif __has_include(<coroutine>) // C++ 20
#include <coroutine>
#else
#error "expect header <experimental/coroutine> or <coroutine/frame.h>"
#endif

#if __has_include(<threadpoolapiset.h>)
#include <Windows.h>
#include <threadpoolapiset.h>

namespace coro {
using namespace std;
using namespace std::experimental;

// Move into the win32 thread pool and continue the routine
class ptp_work final {
    // Callback for CreateThreadpoolWork
    static void __stdcall resume_on_thread_pool(PTP_CALLBACK_INSTANCE, PVOID,
                                                PTP_WORK);

    _INTERFACE_
    auto create_and_submit_work(coroutine_handle<void>) noexcept -> uint32_t;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
        // nothing to do for this implementation
    }
    // Lazy code generation in importing code by header usage.
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        if (const auto ec = create_and_submit_work(coro))
            throw system_error{static_cast<int>(ec), system_category(),
                               "CreateThreadpoolWork"};
    }
};

// Move into the designated thread's APC queue and  and continue the routine
class procedure_call_on final {
    // Callback for QueueUserAPC
    static void __stdcall resume_on_apc(ULONG_PTR);

    _INTERFACE_
    auto queue_user_apc(coroutine_handle<void>) noexcept -> uint32_t;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        if (const auto ec = queue_user_apc(coro))
            throw system_error{static_cast<int>(ec), system_category(),
                               "QueueUserAPC"};
    }

  public:
    explicit procedure_call_on(HANDLE hThread) noexcept : thread{hThread} {
    }

  private:
    HANDLE thread;
};

} // namespace coro

#elif __has_include(<pthread.h>)
#include <pthread.h>

namespace coro {
using namespace std;
using namespace std::experimental;

// Creates a new POSIX Thread and resume the given coroutine handle on it.
class pthread_spawner_t {
    _INTERFACE_
    void resume_on_pthread(coroutine_handle<void> rh) noexcept(false);

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
    }
    void await_suspend(coroutine_handle<void> rh) noexcept(false) {
        return resume_on_pthread(rh);
    }

  public:
    pthread_spawner_t(pthread_t* _tid, //
                      const pthread_attr_t* _attr)
        : tid{_tid}, attr{_attr} {
    }
    ~pthread_spawner_t() noexcept = default;

  private:
    pthread_t* const tid;
    const pthread_attr_t* const attr;
};

// Works like the `ptp_work` of win32 based interface, but rely on the pthread
class ptp_work final : public pthread_spawner_t {
    pthread_t tid;

  public:
    ptp_work() noexcept : pthread_spawner_t{&tid, nullptr}, tid{} {};
};

// This is a special promise type which allows `pthread_attr_t*` as an
// operand of `co_await` operator.
// The type wraps `pthread_create` function. After spawn, it contains thread id
// of the brand-new thread.
class pthread_spawn_promise {
  public:
    pthread_t tid{};

  public:
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    void unhandled_exception() noexcept(false) {
        throw; // the activator is responsible for the exception handling
    }

    auto await_transform(const pthread_attr_t* attr) noexcept(false) {
        if (tid) // already created.
            throw logic_error{"pthread's spawn must be used once"};

        // provide the address at this point
        return pthread_spawner_t{addressof(this->tid), attr};
    }
    inline auto await_transform(pthread_attr_t* attr) noexcept(false) {
        return await_transform(static_cast<const pthread_attr_t*>(attr));
    }

    // general co_await
    template <typename Awaitable>
    decltype(auto) await_transform(Awaitable&& a) noexcept {
        return std::forward<Awaitable&&>(a);
    }
};

// A proxy to `pthread_spawn_promise`.
// The type must ensure the promise contains a valid `pthread_t` when it is
// constructed.
class pthread_knower_t {
  public:
    operator pthread_t() const noexcept {
        // we can access to the `tid` through the pointer
        return promise->tid;
    }

  protected:
    explicit pthread_knower_t(pthread_spawn_promise* _promise) noexcept
        : promise{_promise} {
    }

  protected:
    pthread_spawn_promise* promise;
};

// Special return type that wraps `pthread_join`
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
    // throws `system_error`
    _INTERFACE_ void try_join() noexcept(false);

  public:
    ~pthread_joiner_t() noexcept(false) {
        this->try_join();
    }
    // throws `invalid_argument` for `nullptr`
    _INTERFACE_ pthread_joiner_t(promise_type* p) noexcept(false);
};

// Special return type that wraps `pthread_detach`
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
    // throws `system_error`
    _INTERFACE_ void try_detach() noexcept(false);

  public:
    ~pthread_detacher_t() noexcept(false) {
        this->try_detach();
    }
    // throws `invalid_argument` for `nullptr`
    _INTERFACE_ pthread_detacher_t(promise_type* p) noexcept(false);
};

} // namespace coro

#endif
#endif // COROUTINE_THREAD_UTILITY_H
