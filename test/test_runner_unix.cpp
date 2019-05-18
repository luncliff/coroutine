//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

#include <catch2/catch.hpp>

extern void run_test_with_catch2(test_adapter* test);

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)

#elif defined(_MSC_VER) // Windows
static_assert(false, "this test file can't be used for windows")
#endif                  // Platform specific running

#include "test_concrt_unix.cpp"

TEST_CASE_METHOD(concrt_event_when_no_waiting_test, //
                 "no waiting event", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_event_wait_for_one_test, //
                 "wait for one event", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_event_multiple_wait_on_event_test, //
                 "wait multiple times on 1 event", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_event_ready_after_signaled_test, //
                 "event is ready after signaled", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_event_signal_multiple_test, //
                 "multiple signal on 1 event", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_event_wait_multiple_events_test, //
                 "wait on multiple events", "[event]") {
    run_test_with_catch2(this);
}
