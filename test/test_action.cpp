/**
 * @author  github.com/luncliff (luncliff@gmail.com)
 * @see     https://gist.github.com/luncliff/1fedae034c001a460e9233ecf0afc25b
 */
#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#if __has_include(<Windows.h>)
#include <Windows.h>
#elif __has_include(<dispatch/dispatch.h>)
#include <dispatch/dispatch.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#if __has_include(<catch2/catch_all.hpp>)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include <spdlog/spdlog.h>

#include "action.hpp"

using namespace coro;
using namespace std::chrono_literals;
using namespace std::chrono;

struct fire_and_forget_test_case {
    /**
     * @note Compiler will warn if this function is `noexcept`...
     * @see fire_and_forget::unhandled_exception 
     * @details `unhandled_exception` will rethrow and the exception should be handled by the caller here
     */
    static fire_and_forget throw_in_coroutine() noexcept(false) {
        co_await suspend_never{};
        throw std::runtime_error{__func__};
    }
};

TEST_CASE_METHOD(fire_and_forget_test_case, "rethrow unhandled exception", "[exception]") {
    REQUIRE_THROWS(throw_in_coroutine());
}

struct event_reference_test_case {
    event_proxy_t finished{};
    std::atomic_uint32_t counter{};

  public:
    event_reference_test_case() {
        counter.fetch_add(1);
        finished.context = &counter;
        finished.retain = &on_retain;
        finished.release = &on_release;
        finished.signal = &on_signal;
        finished.wait = &on_wait;
    }
    ~event_reference_test_case() {
        counter.fetch_sub(1);
    }

    static void on_retain(void* c) {
        auto& counter = *reinterpret_cast<std::atomic_uint32_t*>(c);
        counter.fetch_add(1);
        spdlog::debug("add: {}", counter.load());
    }
    static void on_release(void* c) {
        auto& counter = *reinterpret_cast<std::atomic_uint32_t*>(c);
        counter.fetch_sub(1);
        spdlog::debug("sub: {}", counter.load());
    }

    static void on_signal(void* c) {
        auto& counter = *reinterpret_cast<std::atomic_uint32_t*>(c);
        spdlog::debug("signal: {}", counter.load());
        return;
    }
    static uint32_t on_wait(void* c, uint32_t us) {
        std::this_thread::sleep_for(std::chrono::microseconds{us});
        auto& counter = *reinterpret_cast<std::atomic_uint32_t*>(c);
        spdlog::debug("wait: {}", counter.load());
        return 1;
    }

    waitable_action_t spawn(uint32_t us) {
        co_await suspend_never{};
        co_return std::this_thread::sleep_for(std::chrono::microseconds{us});
    }
};

TEST_CASE_METHOD(event_reference_test_case, "reference counting of event_proxy_t") {
    constexpr uint32_t timeout = 20'000; // us
    SECTION("direct use") {
        auto action = spawn(timeout);
        action.resume(finished);
        REQUIRE(action.wait(timeout * 2));
    }
    SECTION("move the action") {
        auto a0 = spawn(timeout);
        auto a1 = std::move(a0);
        a1.resume(finished);
        REQUIRE(a1.wait(timeout * 2));
    }
}

#if __has_include(<Windows.h>)

struct event_proxy_test_case {
    HANDLE e = CreateEventW(nullptr, true, false, nullptr);
    event_proxy_t finished{};

  public:
    event_proxy_test_case() {
        finished.context = e;
        finished.signal = reinterpret_cast<event_proxy_t::fn_signal_t>(&SetEvent);
        finished.wait = &on_wait;
    }
    ~event_proxy_test_case() {
        CloseHandle(e);
    }

    static uint32_t on_wait(void* c, uint32_t us) {
        auto retcode = WaitForSingleObject(static_cast<HANDLE>(c), us);
        if (retcode == WAIT_OBJECT_0)
            return 0;
        if (retcode == WAIT_FAILED)
            return GetLastError();
        return retcode;
    }

    waitable_action_t spawn(HANDLE e, uint32_t us) {
        co_await suspend_never{};
        co_return std::this_thread::sleep_for(std::chrono::microseconds{us});
    }
};

TEST_CASE_METHOD(event_proxy_test_case, "use Win32 Event once", "[dispatch]") {
    constexpr uint32_t timeout = 20'000; // us
    auto action = spawn(e, timeout);
    action.resume(finished);
    REQUIRE(action.wait(timeout * 2) == 0);
}

TEST_CASE_METHOD(event_proxy_test_case, "use Win32 Event multiple times", "[dispatch]") {
    constexpr uint32_t timeout = 10'000; // us
    auto repeat = 4;
    while (--repeat) {
        auto action = spawn(e, timeout);
        action.resume(finished);
        REQUIRE(action.wait(timeout * 2) == 0);
    }
}

#elif __has_include(<dispatch/dispatch.h>)

struct queue_awaitable_t final {
    dispatch_queue_t queue;

  public:
    constexpr bool await_ready() const noexcept {
        return queue == nullptr;
    }
    void await_suspend(coroutine_handle<void> rh) noexcept {
        dispatch_async_f(queue, rh.address(), &coro::resume_once);
    }
    constexpr void await_resume() const noexcept {
    }
};
static_assert(std::is_nothrow_copy_constructible_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_copy_assignable_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_move_constructible_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_move_assignable_v<queue_awaitable_t> == true);

struct event_proxy_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    event_proxy_t finished{};

  public:
    event_proxy_test_case() {
        finished.context = semaphore;
        finished.retain = reinterpret_cast<event_proxy_t::fn_access_t>(&dispatch_retain);
        finished.release = reinterpret_cast<event_proxy_t::fn_access_t>(&dispatch_release);
        finished.signal = reinterpret_cast<event_proxy_t::fn_signal_t>(&dispatch_semaphore_signal);
        finished.wait = &on_wait;
    }
    ~event_proxy_test_case() {
        dispatch_release(semaphore);
    }

    // if zero, it's success
    static uint32_t on_wait(void* c, uint32_t us) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::microseconds{us});
        auto timepoint = dispatch_time(DISPATCH_TIME_NOW, duration.count());
        return dispatch_semaphore_wait(static_cast<dispatch_semaphore_t>(c), timepoint);
    }

    waitable_action_t spawn(dispatch_queue_t q, uint32_t us) {
        co_await queue_awaitable_t{q};
        co_return std::this_thread::sleep_for(std::chrono::microseconds{us});
    }
};

TEST_CASE_METHOD(event_proxy_test_case, "use dispatch_semaphore_t once", "[dispatch]") {
    constexpr uint32_t timeout = 20'000; // us
    auto action = spawn(queue, timeout);
    action.resume(finished);
    REQUIRE(action.wait(timeout * 2) == 0);
}

TEST_CASE_METHOD(event_proxy_test_case, "use dispatch_semaphore_t multiple times", "[dispatch]") {
    constexpr uint32_t timeout = 10'000; // us
    auto repeat = 4;
    while (--repeat) {
        auto action = spawn(queue, timeout);
        action.resume(finished);
        REQUIRE(action.wait(timeout * 2) == 0);
    }
}

#endif
