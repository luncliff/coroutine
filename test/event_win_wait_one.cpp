//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

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

auto ptp_event_wait_one_test() {
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
    auto ms = rand() & 0b1111; // at most 16 ms

    set_after_sleep(ev, ms);

    SleepEx(3, true);
    // issue: CI environment runs slowly, so too short timeout might fail
    // wait for 200 ms
    auto ec = WaitForSingleObjectEx(ev, 200, true);
    REQUIRE(ec == WAIT_OBJECT_0);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return ptp_event_wait_one_test();
}

#elif __has_include(<CppUnitTest.h>)
class ptp_event_wait_one : public TestClass<ptp_event_wait_one> {
    TEST_METHOD(test_ptp_event_wait_one) {
        ptp_event_wait_one_test();
    }
};
#endif
