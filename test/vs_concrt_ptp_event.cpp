//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>

#include <array>

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace coro;
using namespace concrt;

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
        if (ev) // if statement because of C6387
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
