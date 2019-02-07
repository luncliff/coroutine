// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/sync.h>

// clang-format off
#include <Windows.h>
#include <sdkddkver.h>
#include <CppUnitTest.h>
#include <threadpoolapiset.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class wait_group_test : public TestClass<wait_group_test>
{
    static void CALLBACK onWork1(PTP_CALLBACK_INSTANCE instance,
                                 wait_group& group, PTP_WORK work) noexcept
    {
        UNREFERENCED_PARAMETER(instance);
        UNREFERENCED_PARAMETER(work);
        group.done();
    }

    TEST_METHOD(wait_group_wait_once)
    {
        wait_group group{};
        PTP_WORK work = ::CreateThreadpoolWork(
            reinterpret_cast<PTP_WORK_CALLBACK>(onWork1), std::addressof(group),
            nullptr);
        Assert::IsNotNull(work);

        constexpr auto num_of_work = 10;

        // fork - join
        group.add(num_of_work);
        for (auto i = 0u; i < num_of_work; ++i)
            ::SubmitThreadpoolWork(work);

        Assert::IsTrue(group.wait(10s));
        ::CloseThreadpoolWork(work);
    }

    TEST_METHOD(wait_group_multiple_wait)
    {
        wait_group group{};

        group.add(1);
        group.done();
        // Ok. since 1 add, 1 done
        Assert::IsTrue(group.wait(10s));

        // Multiple wait is ok and return true
        Assert::IsTrue(group.wait(10s));
    }
};
