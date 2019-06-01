//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto wait_for_multiple_times(event& e, atomic<uint32_t>& counter) -> no_return {
    while (counter-- > 0)
        co_await e;
}

auto concrt_event_multiple_wait_on_event_test() {
    event e1{};
    atomic<uint32_t> counter{};

    counter = 6;
    wait_for_multiple_times(e1, counter);

    auto repeat = 100u; // prevent infinite loop
    REQUIRE(repeat > counter);
    REQUIRE(counter > 0);

    while (counter > 0 && repeat > 0) {
        e1.set();
        // resume if there is available event-waiting coroutines
        for (auto task : signaled_event_tasks())
            task.resume();

        --repeat;
    };
    REQUIRE(counter == 0);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_multiple_wait_on_event_test();
}
#endif
