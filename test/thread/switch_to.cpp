//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <thread>

#include <coroutine/return.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

SCENARIO("switch_to", "[thread][messaging]")
{
    using namespace std;
    using namespace std::literals;
    const auto timeout = 2s;

    WHEN("worker id is zero")
    {
        wait_group group{};
        thread_id_t before, after;

        auto continue_on = [&](thread_id_t target) noexcept(false)->unplug
        {
            switch_to continuation{target}; // 0 means background thread

            before = current_thread_id();
            co_await continuation; // switch to designated thread

            after = current_thread_id();
            group.done();
        };

        auto tid = thread_id_t{0};

        THEN("move to background")
        {
            group.add(1);
            continue_on(tid);

            REQUIRE(group.wait(timeout));
            // Before switching, it was on this thread.
            // After switching, the flow continued on another thread.
            REQUIRE(before == current_thread_id());
            REQUIRE(before != after);
        }
    }

    WHEN("move to same thread id")
    {
        wait_group group{};
        thread_id_t before, after;

        auto continue_on = [&](thread_id_t target) noexcept(false)->unplug
        {
            switch_to continuation{target};

            before = current_thread_id();
            co_await continuation; // switch to designated thread

            after = current_thread_id();
            group.done();
        };

        auto tid = current_thread_id();

        THEN("flow continues")
        {
            group.add(1);
            continue_on(tid);

            REQUIRE(group.wait(timeout));
            REQUIRE(before == tid);
            REQUIRE(after == tid);
        }
    }

    GIVEN("thread worker")
    {
        thread_id_t worker{}, tester = current_thread_id();

        thread th1{[&]() {
            // assign and send a message for synchronization
            message_t msg{};
            worker = current_thread_id();
            REQUIRE(post_message(tester, msg));

            // fetch 1 coroutine and exit
            std::experimental::coroutine_handle<void> coro{};

            while (peek_switched(coro) == false)
                std::this_thread::sleep_for(1ms);

            REQUIRE(coro.address() != nullptr);
            coro.resume();
        }};

        message_t msg{};
        // wait for worker start
        while (peek_message(msg) == false)
            std::this_thread::sleep_for(1ms);

        REQUIRE(worker != thread_id_t{});

        WHEN("specific thread id")
        {
            // must different thread id
            REQUIRE(tester != worker);

            THEN("move to the thread")
            {
                wait_group group{};
                group.add(1);
                thread_id_t receiver{};

                auto continue_on
                    = [&](thread_id_t target) noexcept(false)->unplug
                {
                    CAPTURE(target);
                    co_await switch_to{target}; // switch to designated thread

                    receiver = current_thread_id();
                    group.done();
                };

                continue_on(worker);

                REQUIRE(group.wait(timeout));
                REQUIRE(receiver != tester);
                REQUIRE(receiver == worker);
            }
        }

        REQUIRE_NOTHROW(th1.join());
    }
}
