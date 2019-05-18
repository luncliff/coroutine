//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

class concrt_event_when_no_waiting_test : public test_adapter {
  public:
    void on_test() override {
        auto count = 0;
        for (auto task : signaled_event_tasks()) {
            task.resume();
            ++count;
        }
        expect_true(count == 0);
    }
};

class concrt_event_wait_for_one_test : public test_adapter {
    static auto wait_for_one_event(event& e, atomic_flag& flag) -> no_return {
        try {
            // resume after the event is signaled ...
            co_await e;
        } catch (system_error& e) {
            // event throws if there was an internal system error
            fail_with_message(e.what());
            co_return;
        }
        flag.test_and_set();
    }

    event e1{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

  public:
    void on_test() override {
        wait_for_one_event(e1, flag);
        e1.set();
        auto count = 0;

        for (auto task : signaled_event_tasks()) {
            task.resume();
            ++count;
        }
        expect_true(count > 0);
        // already set by the coroutine `wait_for_one_event`
        expect_true(flag.test_and_set() == true);
    }
};

class concrt_event_multiple_wait_on_event_test : public test_adapter {

    static auto wait_for_multiple_times(event& e, atomic<uint32_t>& counter)
        -> no_return {
        while (counter-- > 0)
            co_await e;
    }

    event e1{};
    atomic<uint32_t> counter{};

  public:
    void on_test() override {
        counter = 6;
        wait_for_multiple_times(e1, counter);

        auto repeat = 100u; // prevent infinite loop
        expect_true(repeat > counter);
        expect_true(counter > 0);

        while (counter > 0 && repeat > 0) {
            e1.set();
            // resume if there is available event-waiting coroutines
            for (auto task : signaled_event_tasks())
                task.resume();

            --repeat;
        };
        expect_true(counter == 0);
    }
};

class concrt_event_ready_after_signaled_test : public test_adapter {
  public:
    void on_test() override {
        event e1{};
        e1.set(); // when the event is signaled,
                  // `co_await` on it must proceed without suspend
        expect_true(e1.await_ready() == true);
    }
};

class concrt_event_signal_multiple_test : public test_adapter {
  public:
    void on_test() override {
        event e1{};

        e1.set();
        expect_true(e1.await_ready() == true);

        e1.set();
        e1.set();
        expect_true(e1.await_ready() == true);
    }
};

class concrt_event_wait_multiple_events_test : public test_adapter {

    static auto wait_three_events(event& e1, event& e2, event& e3,
                                  atomic_flag& flag) -> no_return {

        co_await e1;
        co_await e2;
        co_await e3;

        flag.test_and_set();
    }

    array<event, 3> events{};
    atomic_flag flag = ATOMIC_FLAG_INIT;

  public:
    void on_test() override {
        wait_three_events(events[0], events[1], events[2], flag);

        // signal in descending order. notice the order in the test function ...
        for (auto idx : {2, 1, 0}) {
            events[idx].set();
            // resume if there is available event-waiting coroutines
            for (auto task : signaled_event_tasks())
                task.resume();
        }
        // set by the coroutine `wait_three_events`
        expect_true(flag.test_and_set() == true);
    }
};
