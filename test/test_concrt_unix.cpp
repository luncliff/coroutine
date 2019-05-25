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
}

auto wait_for_one_event(event& e, atomic_flag& flag) -> no_return {
    try {
        // resume after the event is signaled ...
        co_await e;
    } catch (system_error& e) {
        // event throws if there was an internal system error
        FAIL_WITH_MESSAGE(e.what());
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
}

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
}

auto concrt_event_ready_after_signaled_test() {
    event e1{};
    e1.set(); // when the event is signaled,
              // `co_await` on it must proceed without suspend
    REQUIRE(e1.await_ready() == true);
}

auto concrt_event_signal_multiple_test() {
    event e1{};

    e1.set();
    REQUIRE(e1.await_ready() == true);

    e1.set();
    e1.set();
    REQUIRE(e1.await_ready() == true);
}

static auto wait_three_events(event& e1, event& e2, event& e3,
                              atomic_flag& flag) -> no_return {

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
}

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("no waiting event", "[event]") {
    concrt_event_when_no_waiting_test();
}
TEST_CASE("wait for one event", "[event]") {
    concrt_event_wait_for_one_test();
}
TEST_CASE("wait multiple times on 1 event", "[event]") {
    concrt_event_multiple_wait_on_event_test();
}
TEST_CASE("event is ready after signaled", "[event]") {
    concrt_event_ready_after_signaled_test();
}
TEST_CASE("multiple signal on 1 event", "[event]") {
    concrt_event_signal_multiple_test();
}
TEST_CASE("wait on multiple events", "[event]") {
    concrt_event_wait_multiple_events_test();
}
#endif
