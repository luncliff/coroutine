//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace coro;
using latch_t = concrt::latch;

auto concrt_latch_wait_multiple_test() {
    latch_t wg{1};
    _require_(wg.is_ready() == false);

    wg.count_down_and_wait();
    _require_(wg.is_ready());
    _require_(wg.is_ready()); // mutliple test is ok

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return concrt_latch_wait_multiple_test();
}

#elif __has_include(<CppUnitTest.h>)
class concrt_latch_wait_multiple
    : public TestClass<concrt_latch_wait_multiple> {
    TEST_METHOD(test_concrt_latch_wait_multiple) {
        concrt_latch_wait_multiple_test();
    }
};
#endif
