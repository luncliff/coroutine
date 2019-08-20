//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>
#include <coroutine/return.h>

#include <array>
#include <atomic>

using namespace std;
using namespace coro;

auto wait_three_events(auto_reset_event& e1, auto_reset_event& e2,
                       auto_reset_event& e3, atomic<uint32_t>& flag)
    -> forget_frame {
    co_await e1;
    co_await e2;
    co_await e3;
    flag = 1;
}

auto concrt_event_wait_multiple_events_test() {
    array<auto_reset_event, 3> events{};
    atomic<uint32_t> flag{};
    wait_three_events(events[0], events[1], events[2], flag);

    // signal in descending order. notice the order in the test function ...
    for (auto idx : {2, 1, 0}) {
        events[idx].set();
        // resume if there is available event-waiting coroutines
        for (auto task : signaled_event_tasks())
            task.resume();
    }
    // set by the coroutine `wait_three_events`
    if (flag != 1)
        return __LINE__;

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_wait_multiple_events_test();
}
#endif
