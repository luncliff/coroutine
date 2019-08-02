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

#ifndef CONCURRENCY_HELPER_THREAD_H
#define CONCURRENCY_HELPER_THREAD_H

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

#if __has_include(<Windows.h>)
#include <Windows.h>

namespace coro {
using namespace std;
using namespace std::experimental;

//  Move into the win32 thread pool and continue the routine
class ptp_work final {
    // PTP_WORK_CALLBACK
    static void __stdcall resume_on_thread_pool(PTP_CALLBACK_INSTANCE, PVOID,
                                                PTP_WORK);

    _INTERFACE_ auto on_suspend(coroutine_handle<void>) noexcept -> uint32_t;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
        // nothing to do for this implementation
    }
    // Lazy code generation in importing code by header usage.
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        if (const auto ec = on_suspend(coro))
            throw system_error{static_cast<int>(ec), system_category(),
                               "CreateThreadpoolWork"};
    }
};

} // namespace coro

#elif __has_include(<pthread.h>)
#include <pthread.h>
#include <future>

namespace coro {
using namespace std;
using namespace std::experimental;

// work like `ptp_work` of win32 based interface, but rely on standard
class ptp_work final {
    coroutine_handle<void> rh;

  private:
    void on_suspend(coroutine_handle<void> handle) noexcept(false) {
        rh = handle;
        async(launch::async, [handle]() mutable { handle.resume(); });
    }
    void on_resume() noexcept {
        rh = nullptr; // forget it
    }

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(coroutine_handle<void> handle) noexcept(false) {
        return this->on_suspend(handle);
    }
    void await_resume() noexcept {
        return this->on_resume();
    }
};

//  Special return type that wraps `pthread_create` and `pthread_join`
class pthread_joiner_t final {
  public:
    class promise_type;

    class pthread_spawner_t final {
        friend class promise_type;

        _INTERFACE_ static void* resume_on_pthread(void* ptr) noexcept(false);

        // throws `system_error`
        _INTERFACE_ void on_suspend(coroutine_handle<void> rh) noexcept(false);

      public:
        constexpr bool await_ready() const noexcept {
            return false;
        }
        constexpr void await_resume() noexcept {
        }
        void await_suspend(coroutine_handle<void> rh) noexcept(false) {
            return on_suspend(rh);
        }

      private:
        pthread_spawner_t(pthread_t* _tid, const pthread_attr_t* _attr)
            : tid{_tid}, attr{_attr} {
        }

      public:
        ~pthread_spawner_t() noexcept = default;

      private:
        pthread_t* const tid;
        const pthread_attr_t* const attr;
    };

    class promise_type final {
      public:
        auto initial_suspend() noexcept {
            return suspend_never{};
        }
        auto final_suspend() noexcept {
            return suspend_always{};
        }
        void unhandled_exception() noexcept(false) {
            throw; // the activator is responsible for the error handling
        }
        void return_void() noexcept {
            // we already returns coroutine's frame.
            // so `co_return` can't have its operand
        }
        auto get_return_object() noexcept -> promise_type* {
            return this;
        }

        // throws `logic_error`
        auto await_transform(const pthread_attr_t* attr) noexcept(false) {
            if (tid)
                // already created.
                throw logic_error{"co_await must be used once"};

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

      public:
        pthread_t tid{};
    };

  public:
    operator pthread_t() const noexcept {
        // we can access to the `tid` through the pointer
        return promise->tid;
    }

  public:
    // we will receive the pointer from `get_return_object`
    // throws `invalid_argument` for `nullptr`
    _INTERFACE_ pthread_joiner_t(promise_type* p) noexcept(false);
    // throws `system_error`
    _INTERFACE_ ~pthread_joiner_t() noexcept(false);

  private:
    promise_type* promise;
};

} // namespace coro

#endif
#endif // CONCURRENCY_HELPER_THREAD_H