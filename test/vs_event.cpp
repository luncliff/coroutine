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
        // wait for 20 ms
        auto ec = WaitForSingleObjectEx(e1, 20, true);
        Assert::IsTrue(ec == WAIT_OBJECT_0);
    }
    TEST_METHOD(ptp_work_wait_multiple_event) {
        for (auto e : events) {
            auto ms = rand() & 0b1111; // at most 16 ms
            set_after_sleep(e, ms);
        }
        // wait for 30 ms
        auto ec = WaitForMultipleObjectsEx(events.max_size(), events.data(),
                                           true, 30, true);
        Assert::IsTrue(ec == WAIT_OBJECT_0);
    }
};

class ptp_event_test : public TestClass<ptp_event_test> {

    // we can't use rvalue reference. this design is necessary because
    // `ptp_event` uses INFINITE wait internally.
    // so, with the reference, user must sure one of `SetEvent` or `cancel` will
    // happen in the future
    static auto wait_an_event(ptp_event& token, atomic_flag& flag)
        -> no_return {
        // wait for set or cancel
        // `co_await` will forward `GetLastError` if canceled.
        if (DWORD ec = co_await token) {
            auto emsg = system_category().message(ec);
            Logger::WriteMessage(emsg.c_str());
            return;
        }
        flag.test_and_set();
    }

    HANDLE ev;

    TEST_METHOD_INITIALIZE(create_event) {
        ev = CreateEventEx(nullptr, nullptr, //
                           CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        Assert::IsTrue(ev != NULL);
        ResetEvent(ev);
    }
    TEST_METHOD_CLEANUP(close_event) {
        CloseHandle(ev);
    }
    TEST_METHOD(ptp_event_set_event) {
        ptp_event token{ev};
        atomic_flag flag = ATOMIC_FLAG_INIT;

        wait_an_event(token, flag);
        SetEvent(ev);

        Sleep(3); // wait for background thread
        Assert::IsTrue(flag.test_and_set());
    }
    TEST_METHOD(ptp_event_cancel) {
        ptp_event token{ev};
        atomic_flag flag = ATOMIC_FLAG_INIT;

        wait_an_event(token, flag);
        token.cancel();

        Sleep(3); // wait for background thread
        Assert::IsFalse(flag.test_and_set());
    }
};
