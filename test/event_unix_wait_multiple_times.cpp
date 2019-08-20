//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>
#include <coroutine/return.h>

#include <atomic>
using namespace std;
using namespace coro;

auto wait_for_multiple_times(auto_reset_event& e, atomic<uint32_t>& counter)
    -> forget_frame {
    while (counter-- > 0)
        co_await e;
}

auto concrt_event_multiple_wait_on_event_test() {
    auto_reset_event e1{};
    atomic<uint32_t> counter{};

    counter = 6;
    wait_for_multiple_times(e1, counter);

    auto repeat = 100u; // prevent infinite loop
    while (counter > 0 && repeat > 0) {
        e1.set();
        // resume if there is available event-waiting coroutines
        for (auto task : signaled_event_tasks())
            task.resume();

        --repeat;
    };
    if (counter != 0)
        return __LINE__;

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_multiple_wait_on_event_test();
}
#endif
