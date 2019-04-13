//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <coroutine/suspend.h>

#include <atomic>
#include <gsl/gsl>

#include "./suspend_test.h"

using namespace std;
using namespace std::literals;
using namespace std::experimental;
using namespace coro;

TEST_CASE("suspend_hook", "[return]")
{
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
    auto sq = make_lock_queue();

    SECTION("no await")
    {
        // do nothing with this awaitable object
        auto aw = push_to(*sq);
        REQUIRE(aw.await_ready() == false);
    }

    SECTION("no thread")
    {
        auto routine
            = [](limited_lock_queue& queue, int& status) -> return_ignore {
            status = 1;
            co_await push_to(queue);
            status = 2;
            co_await push_to(queue);
            status = 3;
        };

        auto status = 0;
        routine(*sq, status);
        REQUIRE(status == 1);

        // we know there is a waiting coroutine
        auto coro = pop_from(*sq);
        REQUIRE(coro);
        REQUIRE_NOTHROW(coro.resume());
        REQUIRE(status == 2);

        coro = pop_from(*sq);
        REQUIRE(coro);
        REQUIRE_NOTHROW(coro.resume());
        REQUIRE(status == 3);
    }

    SECTION("one worker thread")
    {
        // do work with 1 thread
        thread worker{[](limited_lock_queue* queue) {
                          auto coro = pop_from(*queue);
                          while (coro == false)
                          {
                              this_thread::sleep_for(1s);
                              coro = pop_from(*queue);
                          }
                          REQUIRE_NOTHROW(coro.resume());
                      },
                      sq.get()};

        auto routine = [](limited_lock_queue& queue, //
                          thread_id_t& invoke_id,    //
                          thread_id_t& resume_id) -> return_ignore {
            invoke_id = get_current_thread_id();
            co_await push_to(queue);
            resume_id = get_current_thread_id();
        };

        thread_id_t id1{}, id2{};
        routine(*sq, id1, id2);

        REQUIRE_NOTHROW(worker.join());
        REQUIRE(id1 != id2);                     // resume id == worker thread
        REQUIRE(id1 == get_current_thread_id()); // invoke id == this thread
    }

    SECTION("multiple worker thread")
    {
        auto resume_only_one = [](limited_lock_queue* queue) {
            void* ptr = nullptr;
            size_t retry_count = 10;
            while (retry_count--)
                if (queue->wait_pop(ptr))
                    break;
                else
                    this_thread::sleep_for(500ms);

            if (retry_count == 0)
                FAIL("failed to pop from suspend queue");

            auto coro = coroutine_handle<void>::from_address(ptr);
            REQUIRE(coro);
            REQUIRE(coro.done() == false);
            coro.resume();
        };
        auto routine = [](limited_lock_queue& queue,
                          std::atomic<size_t>& ref) -> return_ignore {
            // just wait schedule
            co_await push_to(queue);
            ref += 1;
        };

        std::atomic<size_t> count{};
        thread w1{resume_only_one, sq.get()}, // do work with 3 thread
            w2{resume_only_one, sq.get()}, w3{resume_only_one, sq.get()};

        routine(*sq, count); // spawn 3 (num of worker) coroutines
        routine(*sq, count);
        routine(*sq, count);
        // all workers must resumed their own tasks
        REQUIRE_NOTHROW(w1.join(), w2.join(), w3.join());
        REQUIRE(count == 3);
    }

    // test end
}
