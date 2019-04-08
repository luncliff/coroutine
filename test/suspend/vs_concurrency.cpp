//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concurrency_adapter.h>
#include <coroutine/return.h>

// clang-format off
#include <Windows.h>
#include <sdkddkver.h>
#include <CppUnitTest.h>
#include <threadpoolapiset.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class latch_test : public TestClass<latch_test>
{
    using latch = cc::latch;

    static auto create_work(latch& group)
    {
        PTP_WORK work = ::CreateThreadpoolWork(
            [](PTP_CALLBACK_INSTANCE, void* ctx, PTP_WORK work) {
                // cast the context and count down
                auto group = reinterpret_cast<latch*>(ctx);
                group->count_down();

                // automatically remove the work item
                ::CloseThreadpoolWork(work);
            },
            std::addressof(group), nullptr);

        Assert::IsNotNull(work);
        return work;
    }

    TEST_METHOD(latch_ready_after_wait)
    {
        constexpr auto num_of_work = 1u;
        latch group{num_of_work};

        // fork - join
        for (auto i = 0u; i < num_of_work; ++i)
        {
            auto work = create_work(group);
            ::SubmitThreadpoolWork(work);
        }

        group.wait();
        Assert::IsTrue(group.is_ready());
    }

    TEST_METHOD(latch_count_down_and_wait)
    {
        latch group{1};
        Assert::IsTrue(group.is_ready() == false);

        group.count_down_and_wait();
        Assert::IsTrue(group.is_ready());
    }

    TEST_METHOD(latch_throws_for_negative_1)
    {
        latch group{1};
        try
        {
            group.count_down(2);
        }
        catch (const std::underflow_error&)
        {
            return;
        }
        Assert::Fail();
    }

    TEST_METHOD(latch_throws_for_negative_2)
    {
        latch group{0};
        try
        {
            group.count_down_and_wait();
        }
        catch (const std::underflow_error&)
        {
            return;
        }
        Assert::Fail();
    }
};

class barrier_test : public TestClass<barrier_test>
{
    using barrier = cc::barrier;

    static auto create_work(barrier& b)
    {
        PTP_WORK work = ::CreateThreadpoolWork(
            [](PTP_CALLBACK_INSTANCE, void* ctx, PTP_WORK work) {
                // cast the context and count down
                auto b = reinterpret_cast<barrier*>(ctx);

                b->arrive_and_wait();

                // automatically remove the work item
                ::CloseThreadpoolWork(work);
            },
            std::addressof(b), nullptr);

        Assert::IsNotNull(work);
        return work;
    }

    TEST_METHOD(barrier_arrive_and_wait)
    {
        constexpr auto num_of_work = 10u;
        barrier b{num_of_work};

        // spawn num_of_work - 1
        for (auto i = 1u; i < num_of_work; ++i)
        {
            auto work = create_work(b);
            ::SubmitThreadpoolWork(work);
        }

        // this is the last wait
        b.arrive_and_wait();
    }
};