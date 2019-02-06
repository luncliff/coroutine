//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <thread>

#include <coroutine/return.h>
#include <coroutine/suspend_queue.h>
#include <coroutine/sync.h>

using namespace std;
using namespace std::literals;

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

        using thread_id_t = std::thread::id;

        auto routine = [&sq](thread_id_t& invoke_id,
                             thread_id_t& resume_id) -> return_ignore {
            invoke_id = this_thread::get_id();
            co_await sq.wait();
            resume_id = this_thread::get_id();
        };

        thread_id_t id1{}, id2{};
        routine(id1, id2);

        auto worker_id = worker.get_id();
        REQUIRE_NOTHROW(worker.join());
        REQUIRE(id1 != id2);
        REQUIRE(id1 == this_thread::get_id()); // invoke id == this thread
        REQUIRE(id2 == worker_id);             // resume id == worker thread
    }

    SECTION("multiple worker thread")
    {
        auto resume_only_one = [&sq]() {
            size_t retry_count = 5;
            coroutine_task_t coro{};

            while (sq.try_pop(coro) == false)
                if (retry_count--)
                    this_thread::sleep_for(500ms);
                else
                    FAIL("failed to pop from suspend queue");

            REQUIRE_NOTHROW(coro.resume());
        };

        // do work with 3 thread
        // suspend_queue works in thread-safe manner
        thread w1{resume_only_one}, w2{resume_only_one}, w3{resume_only_one};

        auto routine = [&sq]() -> return_ignore {
            co_await sq.wait(); // just wait schedule
        };

        routine(); // spawn 3 (num of worker) coroutines
        routine();
        routine();

        // all workers must resumed their own tasks
        REQUIRE_NOTHROW(w1.join(), w2.join(), w3.join());
    }

    // test end
}
