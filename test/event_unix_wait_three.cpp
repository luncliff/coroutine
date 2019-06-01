//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto wait_three_events(event& e1, event& e2, event& e3, atomic_flag& flag)
    -> no_return {

    co_await e1;
    co_await e2;
    co_await e3;

    flag.test_and_set();
}

auto concrt_event_wait_multiple_events_test() {

    array<event, 3> events{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_three_events(events[0], events[1], events[2], flag);

    // signal in descending order. notice the order in the test function ...
    for (auto idx : {2, 1, 0}) {
        events[idx].set();
        // resume if there is available event-waiting coroutines
        for (auto task : signaled_event_tasks())
            task.resume();
    }
    // set by the coroutine `wait_three_events`
    REQUIRE(flag.test_and_set() == true);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_wait_multiple_events_test();
}
#endif
