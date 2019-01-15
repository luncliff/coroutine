//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <thread>

#include <coroutine/return.h>
#include <coroutine/switch.h>

SCENARIO("switch_to", "[thread][messaging]")
{
    using namespace std;
    using namespace std::literals;

    GIVEN("switch instance")
    {
        switch_to sw{};

        WHEN("corotuine waits for a switching")
        THEN("scheduler can fetch it")
        {
            // switch_to serialize the suspended coroutines
            // scheduler provides interface to fetch them
            auto& sc = sw.scheduler();
            REQUIRE_FALSE(sc.closed());

            // the task is separated with co_await
            auto separated_task = [&sw](int& status) -> unplug {
                status = 1;
                co_await sw;
                status = 2;
                co_await sw;
                status = 3;
            };

            int status = 0;
            separated_task(status);
            REQUIRE(status == 1);

            // in this case, we know there is a waiting coroutine.
            // so we don't have to wait. fetch immediately.
            auto coro = sc.wait(0us);
            REQUIRE(coro);
            REQUIRE_NOTHROW(coro.resume());
            REQUIRE(status == 2);

            // same with above
            coro = sc.wait(0us);
            REQUIRE(coro);
            REQUIRE_NOTHROW(coro.resume());
            REQUIRE(status == 3);
        }
    }

    auto resume_tasks = [](scheduler_t* sc) {
        try
        {
            while (sc->closed() == false)
                // for timeout, `wait` returns empty handle
                if (auto coro = sc->wait(1ms))
                    coro.resume();
        }
        catch (const std::runtime_error& ex)
        {
            // scheduler throws exception for critical error
            CAPTURE(ex.what());
        }
    };

    GIVEN("a worker thread")
    {
        switch_to sw{};
        thread w{resume_tasks, addressof(sw.scheduler())};

        WHEN("no switching")
        THEN("no exception on worker thread")
        {
            // do nothing with the switch_to instance
        }

        WHEN("switch to worker thread")
        THEN("get different thread id")
        {
            wait_group group{};
            thread_id_t before{};
            thread_id_t after{};
            group.add(1);

            [&group,
             &sw ](thread_id_t & th1, thread_id_t & th2) noexcept(false)->unplug
            {
                th1 = current_thread_id();
                // switch to designated thread
                co_await sw;
                th2 = current_thread_id();
                group.done();
            }
            (before, after);

            REQUIRE(group.wait(2s));
            // Before switching, it was on this thread.
            // After switching, the flow continued on another thread.
            REQUIRE(before == current_thread_id());
            REQUIRE(before != after);
        }

        sw.scheduler().close();
        REQUIRE_NOTHROW(w.join());
    }

    GIVEN("multiple worker threads")
    {
        switch_to worker1{};
        switch_to worker2{};
        thread w1{resume_tasks, addressof(worker1.scheduler())};
        thread w2{resume_tasks, addressof(worker2.scheduler())};

        WHEN("access without data-race")
        {
            constexpr size_t repeat_count = 1000u;
            wait_group group{};

            auto pipeline_task = [&group, &worker1, &worker2](
                                     size_t& ctr1, size_t& ctr2) -> unplug {
                co_await worker1;
                ctr1 += 17;

                co_await worker2;
                ctr2 += 13;

                group.done();
            };

            size_t counter1{}, counter2{};
            auto repeat = repeat_count;

            group.add(repeat_count);
            while (repeat--)
                pipeline_task(counter1, counter2);

            group.wait(30s);
            CAPTURE(counter1);
            CAPTURE(counter2);

            // the counters are not atomic, but the serialization worked
            REQUIRE(counter1 == 17 * repeat_count);
            REQUIRE(counter2 == 13 * repeat_count);
        }

        worker1.scheduler().close();
        worker2.scheduler().close();
        REQUIRE_NOTHROW(w1.join(), w2.join());
    }
}
