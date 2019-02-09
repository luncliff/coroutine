//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <coroutine/suspend.h>
#include <coroutine/sync.h>

#include <atomic>
#include <gsl/gsl>

#include "./suspend_test.h"

using namespace std;
using namespace std::literals;

TEST_CASE("suspend_hook", "[return]")
{
    using namespace std::experimental;

    suspend_hook hk{};

    SECTION("empty")
    {
        auto coro = static_cast<coroutine_handle<void>>(hk);
        REQUIRE(coro.address() == nullptr);
    }

    SECTION("resume via coroutine handle")
    {
        gsl::index status = 0;

        auto routine = [=](suspend_hook& hook, auto& status) -> return_ignore {
            auto defer = gsl::finally([&]() {
                // ensure final action
                status = 3;
            });

            status = 1;
            co_await suspend_never{};
            co_await hook;
            status = 2;
            co_await hook;
            co_return;
        };

        REQUIRE_NOTHROW(routine(hk, status));
        REQUIRE(status == 1);
        auto coro = static_cast<coroutine_handle<void>>(hk);
        coro.resume();

        REQUIRE(status == 2);
        coro.resume();

        // coroutine reached end.
        // so `defer` in the routine must be destroyed
        REQUIRE(status == 3);
    }

    // test end
}

TEST_CASE("suspend_queue", "[suspend][thread]")
{
    suspend_queue sq{};

    SECTION("no await")
    {
        // do nothing with this awaitable object
        auto aw = sq.wait();
        REQUIRE(aw.await_ready() == false);
    }

    SECTION("no thread")
    {
        auto routine = [&sq](int& status) -> return_ignore {
            status = 1;
            co_await sq.wait();
            status = 2;
            co_await sq.wait();
            status = 3;
        };

        int status = 0;
        routine(status);
        REQUIRE(status == 1);

        coroutine_task_t coro{};

        // we know there is a waiting coroutine
        REQUIRE(sq.try_pop(coro));
        REQUIRE_NOTHROW(coro.resume());
        REQUIRE(status == 2);

        REQUIRE(sq.try_pop(coro));
        REQUIRE_NOTHROW(coro.resume());
        REQUIRE(status == 3);
    }

    SECTION("one worker thread")
    {
        // do work with 1 thread
        thread worker{[&sq]() {
            coroutine_task_t coro{};

            while (sq.try_pop(coro) == false)
                this_thread::sleep_for(1s);

            REQUIRE_NOTHROW(coro.resume());
        }};

        auto routine = [&sq](thread_id_t& invoke_id,
                             thread_id_t& resume_id) -> return_ignore {
            invoke_id = get_current_thread_id();
            co_await sq.wait();
            resume_id = get_current_thread_id();
        };

        thread_id_t id1{}, id2{};
        routine(id1, id2);

        REQUIRE_NOTHROW(worker.join());
        REQUIRE(id1 != id2);                     // resume id == worker thread
        REQUIRE(id1 == get_current_thread_id()); // invoke id == this thread
    }

    SECTION("multiple worker thread")
    {
        auto resume_only_one = [&sq]() {
            size_t retry_count = 50;
            coroutine_task_t coro{};

            while (sq.try_pop(coro) == false)
                if (retry_count--)
                    this_thread::sleep_for(500ms);
                else
                    FAIL("failed to pop from suspend queue");

            REQUIRE(coro.done() == false);
            coro.resume();
        };

        // do work with 3 thread
        // suspend_queue works in thread-safe manner
        thread w1{resume_only_one}, w2{resume_only_one}, w3{resume_only_one};

        auto routine = [&sq](std::atomic<size_t>& count) -> return_ignore {
            co_await sq.wait(); // just wait schedule
            count += 1;
        };

        std::atomic<size_t> count{};

        routine(count); // spawn 3 (num of worker) coroutines
        routine(count);
        routine(count);

        // all workers must resumed their own tasks
        REQUIRE_NOTHROW(w1.join(), w2.join(), w3.join());
        REQUIRE(count == 3);
    }

    // test end
}
