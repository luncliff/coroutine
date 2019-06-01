//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto concrt_event_when_no_waiting_test() {
    auto count = 0;
    for (auto task : signaled_event_tasks()) {
        task.resume();
        ++count;
    }
    REQUIRE(count == 0);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_when_no_waiting_test();
}
#endif
