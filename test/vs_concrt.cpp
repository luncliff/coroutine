//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <coroutine/concrt.h>
using namespace coro;
using namespace concrt;

#include <array>

class thread_api_test : public TestClass<thread_api_test> {

    TEST_METHOD(thread_of_current_process) {
        auto pid = GetCurrentProcessId();
        bool detected = false;
        // it's simple. find peer threads
        for (auto tid : get_threads(pid)) {
            if (tid == GetCurrentThreadId())
                continue;
            detected = true;
        }
        // since visual studio native test runs with multi-thread,
        // there must be a peer thread for this test process
        Assert::IsTrue(detected);
    }
};

class latch_test : public TestClass<latch_test> {

    static auto spawn_background_work(latch& wg) -> no_return {
        co_await ptp_work{};
        wg.count_down();
    }

    TEST_METHOD(latch_ready_after_wait) {
        constexpr auto num_of_work = 10u;
        latch wg{num_of_work};

        // fork - join
        for (auto i = 0u; i < num_of_work; ++i)
            spawn_background_work(wg);

        wg.wait();
        Assert::IsTrue(wg.is_ready());
    }
    TEST_METHOD(latch_count_down_and_wait) {
        latch wg{1};
        Assert::IsTrue(wg.is_ready() == false);

        wg.count_down_and_wait();
        Assert::IsTrue(wg.is_ready());
    }
    TEST_METHOD(latch_throws_for_negative_1) {
        latch wg{1};
        try {
            wg.count_down(2);
        } catch (const underflow_error&) {
            return;
        }
        Assert::Fail();
    }
    TEST_METHOD(latch_throws_for_negative_2) {
        latch wg{0};
        try {
            wg.count_down_and_wait();
        } catch (const underflow_error&) {
            return;
        }
        Assert::Fail();
    }
};
