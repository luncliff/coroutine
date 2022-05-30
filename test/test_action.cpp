/**
 * @author  github.com/luncliff (luncliff@gmail.com)
 * @see     https://gist.github.com/luncliff/1fedae034c001a460e9233ecf0afc25b
 */
#include <catch2/catch_all.hpp>
#include <spdlog/spdlog.h>

#include <coroutine/action.hpp>
#if __has_include(<dispatch/dispatch.h>)
#include <dispatch/dispatch.h>
#endif

using namespace coro;

struct fire_and_forget_test_case {
    /**
     * @details Compiler will warn if this function is `noexcept`.
     *  However, it will be handled by the return type. 
     *  The uncaught exception won't happen here.
     * 
     * @see fire_and_forget::unhandled_exception 
     */
    static fire_and_forget throw_in_coroutine() noexcept(false) {
        co_await suspend_never{};
        throw std::runtime_error{__func__};
    }
};

TEST_CASE_METHOD(fire_and_forget_test_case, "unhandled exception", "[exception]") {
    REQUIRE_NOTHROW(throw_in_coroutine());
}

#if __has_include(<dispatch/dispatch.h>)

struct paused_action_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  public:
    static paused_action_t spawn0() {
        co_await suspend_never{};
        spdlog::debug("{}", __func__);
        co_return;
    }
};

TEST_CASE_METHOD(paused_action_test_case, "synchronous dispatch", "[dispatch]") {
    paused_action_t action = spawn0();
    coroutine_handle<void> coro = action.handle();
    REQUIRE(coro);
    REQUIRE_FALSE(coro.done());
    // these are synchronous operations with a `dispatch_queue_t`
    SECTION("dispatch_async_and_wait") {
        dispatch_async_and_wait_f(queue, coro.address(), resume_once);
        spdlog::debug("dispatch_async_and_wait");
        REQUIRE(coro.done());
    }
    SECTION("dispatch_barrier_async_and_wait") {
        dispatch_barrier_async_and_wait_f(queue, coro.address(), resume_once);
        spdlog::debug("dispatch_barrier_async_and_wait");
        REQUIRE(coro.done());
    }
    SECTION("dispatch_sync") {
        dispatch_sync_f(queue, coro.address(), resume_once);
        spdlog::debug("dispatch_sync");
        REQUIRE(coro.done());
    }
}

#endif
