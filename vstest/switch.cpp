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
using namespace std::literals;

namespace magic
{

TEST_CLASS(SwitchTest)
{
    TEST_METHOD(NoSwitching)
    {
        DWORD target = GetCurrentThreadId();

        auto s = magic::switch_to{target};
        Assert::IsTrue(s.ready());

        s.resume();
    }

    // This limitation depends on the size of thread message queue
    static constexpr auto Limit = 10000;

    static DWORD WINAPI ThreadWork(LPVOID args)
    {
        Assert::IsNotNull(args);

        // Resumeable Handle
        stdex::coroutine_handle<> rh{};
        size_t count = 0;

        do
        {
            // Wait for next work
            Assert::IsTrue(magic::get(rh));
            Assert::IsNotNull(rh.address());
            rh.resume();

        } while (++count < Limit);

        // notify this thread is going to exit
        wait_group *wg = reinterpret_cast<wait_group *>(args);
        wg->done();

        return S_OK;
    };

    TEST_METHOD(ToWorker)
    {
        wait_group group{};
        group.add(1);

        DWORD worker_id = -1;
        HANDLE thread =
            ::CreateThread(nullptr, 4096 * 2,
                           ThreadWork, std::addressof(group),
                           CREATE_SUSPENDED, &worker_id);

        Assert::IsTrue(thread != NULL);
        Assert::IsTrue(worker_id > 0);
        Assert::IsTrue(::ResumeThread(thread) == TRUE);

        // wait for worker thread to run.
        // If main thread uses switch_to before that, the exception will be throwed with
        // ERROR_INVALID_THREAD_ID
        std::this_thread::sleep_for(5s);

        size_t count = 0;
        do
        {
            // Spawn a coroutine for worker
            [](DWORD thread_id) -> unplug {
                try
                {
                    // Go to specific thread ...
                    co_await magic::switch_to{thread_id};
                    Assert::IsTrue(thread_id == GetCurrentThreadId());
                }
                catch (const std::runtime_error &ec)
                {
                    Logger::WriteMessage(ec.what());
                    Assert::IsTrue(GetLastError() == ERROR_INVALID_THREAD_ID);
                    Assert::Fail();
                }
            }(worker_id);

        } while (++count < Limit);

        // wait for worker thread to exit
        group.wait();
        ::CloseHandle(thread);
    }

    TEST_METHOD(TravelThreadPool)
    {
        const DWORD main_thread = GetCurrentThreadId();

        size_t count = Limit;

        // go to background and return home
        while (count--)
        {
            [](DWORD thread_id) -> unplug {
                const DWORD start = GetCurrentThreadId();

                // Go to background ...
                co_await magic::switch_to{};

                // Background thread's ID is different
                Assert::IsTrue(start != GetCurrentThreadId());

                // Go to specific thread ...
                co_await magic::switch_to{thread_id};

                Assert::IsTrue(thread_id == GetCurrentThreadId());
            }(main_thread);
        }

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
