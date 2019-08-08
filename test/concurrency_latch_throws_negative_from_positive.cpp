//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <concurrency_helper.h>
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

auto concrt_latch_throws_when_negative_from_positive_test() {
    try {

        latch wg{1};
        wg.count_down(4);

    } catch (const underflow_error&) {
        return EXIT_SUCCESS;
    }
    _fail_now_("expect exception", __FILE__, __LINE__);
    return EXIT_FAILURE;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coroutine_haconcrt_latch_throws_when_negative_from_positive_testndle_swap_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class concrt_latch_throws_when_negative_from_positive
    : public TestClass<concrt_latch_throws_when_negative_from_positive> {
    TEST_METHOD(test_concrt_latch_throws_when_negative_from_positive) {
        concrt_latch_throws_when_negative_from_positive_test();
    }
};
#endif
