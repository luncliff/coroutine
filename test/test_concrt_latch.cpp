//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using latch_t = concrt::latch;

void count_down_on_latch(latch_t* wg) {
    wg->count_down(); // simply count down
}

auto concrt_latch_wait_after_ready_test() {
    static constexpr auto num_of_work = 10u;
    latch_t wg{num_of_work};
    array<future<void>, num_of_work> contexts{};

    // general fork-join will be similar to this ...
    for (auto& f : contexts)
        f = async(launch::async, count_down_on_latch, addressof(wg));

    wg.wait();
    REQUIRE(wg.is_ready());

    for (auto& f : contexts)
        f.wait();
};

auto concrt_latch_count_down_and_wait_test() {
    latch_t wg{1};
    REQUIRE(wg.is_ready() == false);

    wg.count_down_and_wait();
    REQUIRE(wg.is_ready());
    REQUIRE(wg.is_ready()); // mutliple test is ok
}

auto concrt_latch_throws_when_negative_from_positive_test() {
    latch_t wg{1};
    try {
        wg.count_down(4);
    } catch (const underflow_error&) {
        return;
    }
    FAIL_WITH_MESSAGE("expect exception"s);
}

auto concrt_latch_throws_when_negative_from_zero_test() {
    latch_t wg{0};
    try {
        wg.count_down(2);
    } catch (const underflow_error&) {
        return;
    }
    FAIL_WITH_MESSAGE("expect exception"s);
};

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("latch wait after ready", "[concurrency]") {
    concrt_latch_wait_after_ready_test();
}
TEST_CASE("latch count_down and wait", "[concurrency]") {
    concrt_latch_count_down_and_wait_test();
}
TEST_CASE("latch when underflow 1", "[concurrency]") {
    concrt_latch_throws_when_negative_from_positive_test();
}
TEST_CASE("latch when underflow 2", "[concurrency]") {
    concrt_latch_throws_when_negative_from_zero_test();
}

#elif __has_include(<CppUnitTest.h>)
class concrt_latch_wait_after_ready
    : public TestClass<concrt_latch_wait_after_ready> {
    TEST_METHOD(test_concrt_latch_wait_after_ready) {
        concrt_latch_wait_after_ready_test();
    }
};
class concrt_latch_count_down_and_wait
    : public TestClass<concrt_latch_count_down_and_wait> {
    TEST_METHOD(test_concrt_latch_count_down_and_wait) {
        concrt_latch_count_down_and_wait_test();
    }
};
class concrt_latch_throws_when_negative_from_positive
    : public TestClass<concrt_latch_throws_when_negative_from_positive> {
    TEST_METHOD(test_concrt_latch_throws_when_negative_from_positive) {
        concrt_latch_throws_when_negative_from_positive_test();
    }
};
class concrt_latch_throws_when_negative_from_zero
    : public TestClass<concrt_latch_throws_when_negative_from_zero> {
    TEST_METHOD(test_concrt_latch_throws_when_negative_from_zero) {
        concrt_latch_throws_when_negative_from_zero_test();
    }
};

#endif