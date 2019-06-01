//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto wait_for_one_event(event& e, atomic_flag& flag) -> no_return {
    try {
        // resume after the event is signaled ...
        co_await e;
    } catch (system_error& e) {
        // event throws if there was an internal system error
        FAIL_WITH_MESSAGE(string{e.what()});
        co_return;
    }
    flag.test_and_set();
}

auto concrt_event_wait_for_one_test() {
    event e1{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_for_one_event(e1, flag);
    e1.set();
    auto count = 0;

    for (auto task : signaled_event_tasks()) {
        task.resume();
        ++count;
    }
    REQUIRE(count > 0);
    // already set by the coroutine `wait_for_one_event`
    REQUIRE(flag.test_and_set() == true);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_wait_for_one_test();
}
#endif
