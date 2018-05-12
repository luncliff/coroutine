// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      This file is distributed under Creative Commons 4.0-BY License
//
// ---------------------------------------------------------------------------
#include <magic/sync.h>
#include <magic/switch.h>
#include <magic/channel.hpp>

#include <sdkddkver.h>
#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace magic
{
using namespace std;

TEST_CLASS(SwitchToThreadTest)
{

    auto TravelToBackground(const DWORD thread)->unplug
    {
        const DWORD start = GetCurrentThreadId();

        // Go to background ...
        co_await magic::switch_to{};

        // Background thread's ID is different
        Assert::IsTrue(start != GetCurrentThreadId(),
                       L"Expect differenct thread ID");

        // Go to specific thread ...
        co_await magic::switch_to{thread};

        Assert::IsTrue(thread == GetCurrentThreadId(),
                       L"Reached wrong worker thread");
    };

    TEST_METHOD(SwitchNever)
    {
        DWORD target = GetCurrentThreadId();

        auto s = magic::switch_to{target};
        Assert::IsTrue(s.ready());

        s.resume();
    }

    /*
        TEST_METHOD(SwitchTravelToWorker)
        {
            DWORD target = -1;
            HANDLE thread = CreateThread(nullptr, 4096 * 2, Working, nullptr, 0, &target);

            Assert::IsTrue(thread != INVALID_HANDLE_VALUE,
                           L"Failed to create Worker thread");
            Assert::IsTrue(target != -1,
                           L"Failed to get Worker thread ID");

            // Resumeable Handle
            stdex::coroutine_handle<> rh{};

            // ...

            CloseHandle(thread);
        }
        */

    TEST_METHOD(SwitchTravelMany)
    {
        // This limitation depends on the size of thread message queue
        const size_t Limit = 10000;
        const DWORD home = GetCurrentThreadId();

        size_t count = Limit;

        // go to background and return home
        while (count--)
            TravelToBackground(home);

        count = Limit;
        while (count--)
        {
            // Resumeable Handle
            stdex::coroutine_handle<> rh{};

            // Wait for next work
            Assert::IsTrue(magic::get(rh));
            rh.resume();
        }
    }
};
} // namespace magic
