//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto wait_an_event(ptp_event& token, atomic_flag& flag) -> no_return;
auto set_after_sleep(HANDLE ev, uint32_t ms) -> no_return;

auto ptp_event_cancel_test() {
    array<HANDLE, 10> events{};
    for (auto& e : events) {
        e = CreateEventEx(nullptr, nullptr, //
                          CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        REQUIRE(e != NULL);
        if (e) // if statement because of C6387
            ResetEvent(e);
    }
    auto on_return = gsl::finally([&events]() {
        for (auto e : events)
            CloseHandle(e);
    });
    HANDLE& ev = events[0];
    ptp_event token{ev};
    atomic_flag flag = ATOMIC_FLAG_INIT;

    wait_an_event(token, flag);
    token.cancel();

    SleepEx(3, true); // give time to windows threads
    REQUIRE(flag.test_and_set() == false);

    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class ptp_event_cancel : public TestClass<ptp_event_cancel> {
    TEST_METHOD(test_ptp_event_cancel) {
        ptp_event_cancel_test();
    }
};
#else
int main(int, char* []) {
    return ptp_event_cancel_test();
}

// we can't use rvalue reference. this design is necessary because
// `ptp_event` uses INFINITE wait internally.
// so, with the reference, user must sure one of `SetEvent` or `cancel` will
// happen in the future
auto wait_an_event(ptp_event& token, atomic_flag& flag) -> no_return {
    // wait for set or cancel
    // `co_await` will forward `GetLastError` if canceled.
    if (DWORD ec = co_await token) {
        FAIL_WITH_MESSAGE(system_category().message(ec));
        co_return;
    }
    flag.test_and_set();
}

auto set_after_sleep(HANDLE ev, uint32_t ms) -> no_return {
    co_await ptp_work{}; // move to background thread ...

    SleepEx(ms, true);
    // if failed, print error message
    if (SetEvent(ev) == 0) {
        auto ec = GetLastError();
        FAIL_WITH_MESSAGE(system_category().message(ec));
    }
}
#endif
