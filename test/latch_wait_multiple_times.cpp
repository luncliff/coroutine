//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using latch_t = concrt::latch;

auto concrt_latch_wait_multiple_test() {
    latch_t wg{1};
    REQUIRE(wg.is_ready() == false);

    wg.count_down_and_wait();
    REQUIRE(wg.is_ready());
    REQUIRE(wg.is_ready()); // mutliple test is ok

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_latch_wait_multiple_test();
}
#endif
