//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

#include "test_concrt_windows.cpp"

#define CATCH_CONFIG_FAST_COMPILE
#define CATCH_CONFIG_WINDOWS_CRTDBG
#include <catch2/catch.hpp>

extern void run_test_with_catch2(test_adapter* test);

TEST_CASE_METHOD(thread_api_test, //
                 "concrt thread api utility", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(ptp_event_set_test, //
                 "concrt ptp_event set", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(ptp_event_cancel_test, //
                 "concrt ptp_event cancel", "[event]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(ptp_event_wait_one_test, //
                 "concrt ptp_event wait one", "[event][concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(ptp_event_wait_multiple_test, //
                 "concrt ptp_event wait multiple", "[event][concurrency]") {
    run_test_with_catch2(this);
}
