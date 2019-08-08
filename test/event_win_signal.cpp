//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>
#include <coroutine/return.h>
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto wait_an_event(ptp_event& token, atomic_flag& flag) -> forget_frame;
auto set_after_sleep(HANDLE ev, uint32_t ms) -> forget_frame;

auto ptp_event_set_test() {
    HANDLE e = CreateEventEx(nullptr, nullptr, //
                             CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if (e) // if statement because of C6387
        ResetEvent(e);
    _require_(e != NULL);

    auto on_return = gsl::finally([e]() { CloseHandle(e); });

    ptp_event token{e};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_an_event(token, flag);
    SetEvent(e);

    SleepEx(30, true); // give time to windows threads
    _require_(flag.test_and_set());

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)

int main(int, char* []) {
    return ptp_event_set_test();
}

// we can't use rvalue reference. this design is necessary because
// `ptp_event` uses INFINITE wait internally.
// so, with the reference, user must sure one of `SetEvent` or `cancel` will
// happen in the future
auto wait_an_event(ptp_event& token, atomic_flag& flag) -> forget_frame {
    // wait for set or cancel
    // `co_await` will forward `GetLastError` if canceled.
    if (DWORD ec = co_await token) {
        _fail_now_(system_category().message(ec).c_str(), __FILE__, __LINE__);
        co_return;
    }
    flag.test_and_set();
}

auto set_after_sleep(HANDLE ev, uint32_t ms) -> forget_frame {
    co_await ptp_work{}; // move to background thread ...

    SleepEx(ms, true);
    // if failed, print error message
    if (SetEvent(ev) == 0) {
        auto ec = GetLastError();
        _fail_now_(system_category().message(ec).c_str(), __FILE__, __LINE__);
    }
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class ptp_event_set : public TestClass<ptp_event_set> {
    TEST_METHOD(test_ptp_event_set) {
        ptp_event_set_test();
    }
};
#endif
