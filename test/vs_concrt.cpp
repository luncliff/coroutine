//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>

#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace std::literals;

using namespace std::experimental;
using namespace coro;
using namespace concrt;

class latch_test : public TestClass<latch_test>
{
    using wait_group = concrt::latch;

    static auto spawn_background_work(wait_group& group) -> no_return
    {
        co_await ptp_work{}; // move to background thread ...
        group.count_down();
    }

    TEST_METHOD(latch_ready_after_wait)
    {
        constexpr auto num_of_work = 10u;
        wait_group group{num_of_work};

        // fork - join
        for (auto i = 0u; i < num_of_work; ++i)
            spawn_background_work(group);

        group.wait();
        Assert::IsTrue(group.is_ready());
    }

    TEST_METHOD(latch_count_down_and_wait)
    {
        wait_group group{1};
        Assert::IsTrue(group.is_ready() == false);

        group.count_down_and_wait();
        Assert::IsTrue(group.is_ready());
    }

    TEST_METHOD(latch_throws_for_negative_1)
    {
        wait_group group{1};
        try
        {
            group.count_down(2);
        }
        catch (const underflow_error&)
        {
            return;
        }
        Assert::Fail();
    }

    TEST_METHOD(latch_throws_for_negative_2)
    {
        wait_group group{0};
        try
        {
            group.count_down_and_wait();
        }
        catch (const underflow_error&)
        {
            return;
        }
        Assert::Fail();
    }
};
