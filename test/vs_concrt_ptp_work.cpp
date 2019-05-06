//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>

#include <array>

#include <CppUnitTest.h>

using namespace coro;
using namespace concrt;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class ptp_work_test : public TestClass<ptp_work_test> {

    static auto set_after_sleep(HANDLE ev, uint32_t ms) -> no_return {
        co_await ptp_work{}; // move to background thread ...

        Sleep(ms);
        // if failed, print error message
        if (SetEvent(ev) == 0) {
            auto ec = GetLastError();
            auto m = system_category().message(ec);
            Logger::WriteMessage(m.c_str());
        }
    }

    array<HANDLE, 10> events{};

    TEST_METHOD_INITIALIZE(create_events) {
        for (auto& e : events) {
            e = CreateEventEx(nullptr, nullptr, //
                              CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            Assert::IsTrue(e != NULL);
            if (e) // if statement because of C6387
                ResetEvent(e);
        }
    }
    TEST_METHOD_CLEANUP(close_events) {
        for (auto e : events)
            CloseHandle(e);
    }
    TEST_METHOD(ptp_work_wait_one_event) {
        HANDLE& e1 = events[0];
        auto ms = rand() & 0b1111; // at most 16 ms

        set_after_sleep(e1, ms);

        Sleep(3);
        // issue: CI environment runs slowly, so too short timeout might fail
        // wait for 200 ms
        auto ec = WaitForSingleObjectEx(e1, 200, true);
        Assert::IsTrue(ec == WAIT_OBJECT_0);
    }
    TEST_METHOD(ptp_work_wait_multiple_event) {
        for (auto e : events) {
            auto ms = rand() & 0b1111; // at most 16 ms
            set_after_sleep(e, ms);
        }

        Sleep(3);
        // issue: CI environment runs slowly, so too short timeout might fail
        // wait for 300 ms
        auto ec = WaitForMultipleObjectsEx(events.max_size(), events.data(),
                                           true, 300, true);
        Assert::IsTrue(ec == WAIT_OBJECT_0);
    }
};
