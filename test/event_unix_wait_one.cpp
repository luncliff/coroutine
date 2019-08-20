//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>
#include <coroutine/return.h>

#include <atomic>
#include <system_error>

using namespace std;
using namespace coro;

auto wait_for_one_event(auto_reset_event& e, atomic_flag& flag)
    -> forget_frame {
    try {
        // resume after the event is signaled ...
        co_await e;

        flag.test_and_set();
    } catch (system_error& e) {
        // event throws if there was an internal system error
        std::terminate();
    }
}

auto concrt_event_wait_for_one_test() {
    auto_reset_event e1{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_for_one_event(e1, flag);
    e1.set();

    auto count = 0;
    for (auto task : signaled_event_tasks()) {
        task.resume();
        ++count;
    }
    if (count == 0)
        return __LINE__;
    // already set by the coroutine `wait_for_one_event`
    if (flag.test_and_set() == false)
        return __LINE__;

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_wait_for_one_test();
}
#endif
