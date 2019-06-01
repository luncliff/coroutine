//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto concrt_event_signal_multiple_test() {
    event e1{};

    e1.set();
    REQUIRE(e1.await_ready() == true);

    e1.set();
    e1.set();
    REQUIRE(e1.await_ready() == true);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_signal_multiple_test();
}
#endif
