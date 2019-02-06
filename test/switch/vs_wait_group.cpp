// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <Windows.h>
#include <sdkddkver.h>

#include <CppUnitTest.h>
#include <TlHelp32.h>
#include <threadpoolapiset.h>

using namespace std::literals;
using namespace std::experimental;

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

class wait_group_test : public TestClass<wait_group_test>
{
    static void CALLBACK onWork1(PTP_CALLBACK_INSTANCE instance,
                                 wait_group& group, PTP_WORK work) noexcept
    {
        UNREFERENCED_PARAMETER(instance);
        UNREFERENCED_PARAMETER(work);
        group.done();
    }

    wait_group group{};
    PTP_WORK work{};

    TEST_METHOD_INITIALIZE(setup)
    {
        work = ::CreateThreadpoolWork(
            reinterpret_cast<PTP_WORK_CALLBACK>(onWork1), std::addressof(group),
            nullptr);
        Assert::IsNotNull(work);
    }
    TEST_METHOD_CLEANUP(teardown)
    {
        ::CloseThreadpoolWork(work);
    }

    TEST_METHOD(wait_group_wait_once)
    {
        constexpr auto num_of_work = 1000;

        // fork - join
        group.add(num_of_work);
        for (auto i = 0u; i < num_of_work; ++i)
            ::SubmitThreadpoolWork(work);

        Assert::IsTrue(group.wait(10s));
    }

    TEST_METHOD(wait_group_multiple_wait)
    {
        group.add(1);
        group.done();
        // Ok. since 1 add, 1 done
        Assert::IsTrue(group.wait(10s));

        // Multiple wait is ok and return true
        Assert::IsTrue(group.wait(10s));
    }
};
