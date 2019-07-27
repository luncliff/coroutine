//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace coro;
using latch_t = concrt::latch;

auto concrt_latch_throws_when_negative_from_positive_test() {
    latch_t wg{1};
    try {
        wg.count_down(4);
    } catch (const underflow_error&) {
        return EXIT_SUCCESS;
    }
    FAIL_WITH_MESSAGE("expect exception"s);

    return EXIT_FAILURE;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coroutine_haconcrt_latch_throws_when_negative_from_positive_testndle_swap_test();
}

#elif __has_include(<CppUnitTest.h>)
class concrt_latch_throws_when_negative_from_positive
    : public TestClass<concrt_latch_throws_when_negative_from_positive> {
    TEST_METHOD(test_concrt_latch_throws_when_negative_from_positive) {
        concrt_latch_throws_when_negative_from_positive_test();
    }
};
#endif
