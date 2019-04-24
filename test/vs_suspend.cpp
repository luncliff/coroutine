//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>
#include <coroutine/suspend.h>

#include <atomic>
#include <gsl/gsl>
#include <thread>

#include <CppUnitTest.h>
#include <Windows.h>
#include <sdkddkver.h>

using namespace std;
using namespace std::literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace coro;

class suspend_hook_test : public TestClass<suspend_hook_test>
{
    suspend_hook hk{};

    TEST_METHOD(suspend_hook_empty)
    {
        auto coro = static_cast<coroutine_handle<void>>(hk);
        Assert::IsTrue(coro.address() == nullptr);
    }

    TEST_METHOD(suspend_hook_to_coroutine_handle)
    {
        gsl::index status = 0;

        auto routine = [=](suspend_hook& hook, auto& status) -> return_ignore {
            auto defer = gsl::finally([&]() {
                // ensure final action
                status = 3;
            });

            status = 1;
            co_await std::experimental::suspend_never{};
            co_await hook;
            status = 2;
            co_await hook;
            co_return;
        };

        routine(hk, status);
        Assert::IsTrue(status == 1);

        auto coro = static_cast<coroutine_handle<void>>(hk);
        coro.resume();

        Assert::IsTrue(status == 2);
        coro.resume();

        // coroutine reached end.
        // so `defer` in the routine must be destroyed
        Assert::IsTrue(status == 3);
    }
};

class suspend_queue_test : public TestClass<suspend_queue_test>
{
    TEST_METHOD(suspend_queue_no_await)
    {
        auto sq = make_lock_queue();

        // do nothing with this awaitable object
        auto aw = push_to(*sq);
        Assert::IsTrue(aw.await_ready() == false);
    }

    TEST_METHOD(suspend_queue_no_thread)
    {
        auto sq = make_lock_queue();

        auto routine
            = [](limited_lock_queue& queue, int& status) -> return_ignore {
            status = 1;
            co_await push_to(queue);
            status = 2;
            co_await push_to(queue);
            status = 3;
        };

        int status = 0;
        routine(*sq, status);
        Assert::IsTrue(status == 1);

        auto coro = pop_from(*sq);
        // we know there is a waiting coroutine
        Assert::IsTrue(coro);
        coro.resume();
        Assert::IsTrue(status == 2);

        coro = pop_from(*sq);
        Assert::IsTrue(coro);
        coro.resume();
        Assert::IsTrue(status == 3);
    }

    using thread_id_t = DWORD;

    static void resume_one_from_queue(limited_lock_queue* queue)
    {
        void* ptr = nullptr;
        size_t retry_count = 10; // retry if failed to pop item
        while (retry_count--)
            if (queue->wait_pop(ptr))
                break;
            else
                this_thread::sleep_for(10ms);

        if (retry_count == 0)
            Assert::Fail(L"failed to pop from suspend queue");

        // ok. resume poped coroutine
        auto coro = coroutine_handle<void>::from_address(ptr);
        Assert::IsTrue(coro);
        Assert::IsTrue(coro.done() == false);
        coro.resume();
    };

    TEST_METHOD(suspend_queue_one_worker_thread)
    {
        auto sq = make_lock_queue();

        // do work with 1 thread
        thread worker{resume_one_from_queue, sq.get()};

        auto routine = [](limited_lock_queue& queue,      //
                          atomic<thread_id_t>& invoke_id, //
                          atomic<thread_id_t>& resume_id) -> return_ignore {
            invoke_id = GetCurrentThreadId();
            co_await push_to(queue);
            resume_id = GetCurrentThreadId();
        };

        atomic<thread_id_t> id1{}, id2{};
        routine(*sq, id1, id2);

        worker.join();
        Assert::IsTrue(id1 == GetCurrentThreadId()); // invoke id == this thread
        Assert::IsTrue(id1 != id2); // resume id == worker thread
        // Assert::IsNotNull(worker.native_handle());
    }

    TEST_METHOD(suspend_queue_multiple_worker_thread)
    {
        auto sq = make_lock_queue();

        auto routine = [](limited_lock_queue& queue,
                          atomic<size_t>& ref) -> return_ignore {
            // just wait schedule
            co_await push_to(queue);
            ref += 1;
        };

        atomic<size_t> count{};
        // do work with 3 thread
        thread w1{resume_one_from_queue, sq.get()};
        thread w2{resume_one_from_queue, sq.get()};
        thread w3{resume_one_from_queue, sq.get()};

        routine(*sq, count); // spawn 3 (num of worker) coroutines
        routine(*sq, count);
        routine(*sq, count);
        // all workers must resumed their own tasks
        w1.join();
        w2.join();
        w3.join();
        Assert::IsTrue(count == 3);
    }
};
