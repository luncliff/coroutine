//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto concrt_event_ready_after_signaled_test() {
    event e1{};
    e1.set(); // when the event is signaled,
              // `co_await` on it must proceed without suspend
    REQUIRE(e1.await_ready() == true);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_ready_after_signaled_test();
}
#endif
