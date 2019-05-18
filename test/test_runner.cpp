//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

#include "test_concrt_latch.cpp"
#include "test_coro_return.cpp"
#include "test_coroutine_handle.cpp"

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#define CATCH_CONFIG_FAST_COMPILE
#elif defined(_MSC_VER) // Windows
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif // Platform specific configuration
#include <catch2/catch.hpp>

// simple template method pattern
void run_test_with_catch2(test_adapter* test) {
    test->on_setup();
    REQUIRE_NOTHROW(test->on_test());
    test->on_teardown();
}

TEST_CASE_METHOD(coroutine_handle_move_test, //
                 "coroutine_handle move", "[primitive]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coroutine_handle_swap_test, //
                 "coroutine_handle swap", "[primitive]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_no_return_test, //
                 "no return from coroutine", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_empty_test, //
                 "empty frame object", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_first_suspend_test, //
                 "frame after first suspend", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_awaitable_test, //
                 "frame supports co_await", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_wait_after_ready_test, //
                 "latch wait after ready", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_count_down_and_wait_test, //
                 "latch count_down and wait", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_throws_when_negative_from_positive_test, //
                 "latch when underflow 1", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_throws_when_negative_from_zero_test, //
                 "latch when underflow 2", "[concurrency]") {
    run_test_with_catch2(this);
}
