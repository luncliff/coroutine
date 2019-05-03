//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/concrt.h>
using namespace coro;
using namespace concrt;

#include <array>
#include <atomic>
using namespace std;

auto wait_for_one_event(event& e, atomic_flag& flag) -> no_return {
    try {
        // resume after the event is signaled ...
        co_await e;

    } catch (system_error& e) {
        FAIL(e.what());
    }
    flag.test_and_set();
}

TEST_CASE("wait for one event", "[event]") {
    event e1{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_for_one_event(e1, flag);

    e1.set();
    auto count = 0;
    for (auto task : signaled_event_tasks()) {
        REQUIRE(task.done() == false);
        task.resume();
        ++count;
    }
    REQUIRE(count > 0);
    // already set by the coroutine `wait_for_one_event`
    REQUIRE(flag.test_and_set() == true);
}

auto wait_three_events(event& e1, event& e2, event& e3, atomic_flag& flag)
    -> no_return {

    co_await e1;
    co_await e2;
    co_await e3;

    flag.test_and_set();
}

TEST_CASE("wait for multiple events", "[event]") {
    array<event, 3> events{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_three_events(events[0], events[1], events[2], flag);

    // signal in descending order. notice the order in the test function ...
    for (auto idx : {2, 1, 0}) {
        events[idx].set();

        auto count = 0;
        for (auto task : signaled_event_tasks()) {
            task.resume();
            ++count;
        }
        REQUIRE(count > 0);
    }
    // set by the coroutine `wait_three_events`
    REQUIRE(flag.test_and_set() == true);
}
