//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <thread>

enum thread_id_t : uint64_t;

// - Note
//      Thin wrapper to prevent constant folding of std namespace
//      thread id query
auto get_current_thread_id() noexcept -> thread_id_t;

#if defined(_MSC_VER)
#include <Windows.h>
#include <sdkddkver.h>

thread_id_t get_current_thread_id() noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <pthread.h>

auto get_current_thread_id() noexcept -> thread_id_t
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);
    return static_cast<thread_id_t>(tid);
}
#endif

#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <coroutine/suspend.h>

#include <atomic>
#include <future>
#include <gsl/gsl>

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
        auto routine = [](limited_lock_queue& queue, //
                          int& status) -> return_ignore {
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

    auto resume_one_from_queue = [](limited_lock_queue* queue) {
        void* ptr = nullptr;
        size_t retry_count = 10; // retry if failed to pop item
        while (retry_count--)
            if (queue->wait_pop(ptr))
                break;
            else
                this_thread::sleep_for(500ms);

        if (retry_count == 0)
            FAIL("failed to pop from suspend queue");

        // ok. resume poped coroutine
        auto coro = coroutine_handle<void>::from_address(ptr);
        REQUIRE(coro);
        REQUIRE(coro.done() == false);
        coro.resume();
    };

    SECTION("one worker thread")
    {
        // do work with 1 thread
        auto worker
            = std::async(launch::async, resume_one_from_queue, sq.get());

        auto routine = [](limited_lock_queue& queue, //
                          thread_id_t& invoke_id,    //
                          thread_id_t& resume_id) -> return_ignore {
            invoke_id = get_current_thread_id();
            co_await push_to(queue);
            resume_id = get_current_thread_id();
        };

        thread_id_t id1{}, id2{};
        routine(*sq, id1, id2);

        REQUIRE_NOTHROW(worker.get());
        REQUIRE(id1 != id2);                     // resume id == worker thread
        REQUIRE(id1 == get_current_thread_id()); // invoke id == this thread
    }

    SECTION("multiple worker thread")
    {
        auto routine = [](limited_lock_queue& queue,
                          std::atomic<size_t>& ref) -> return_ignore {
            // just wait schedule
            co_await push_to(queue);
            ref += 1;
        };

        std::atomic<size_t> count{};
        // do work with 3 thread
        auto w1 = std::async(launch::async, resume_one_from_queue, sq.get());
        auto w2 = std::async(launch::async, resume_one_from_queue, sq.get());
        auto w3 = std::async(launch::async, resume_one_from_queue, sq.get());

        routine(*sq, count); // spawn 3 (num of worker) coroutines
        routine(*sq, count);
        routine(*sq, count);
        // all workers must resumed their own tasks
        REQUIRE_NOTHROW(w1.get(), w2.get(), w3.get());
        REQUIRE(count == 3);
    }

    // test end
}
