//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h" ng namespace coro;
using namespace concrt;

#include <coroutine/channel.hpp>event& token, atomic_flag& flag) -> no_return;
auto set_after_sleep(HANDLE ev, uint32_t ms) -> no_return;

auto ptp_event_set_test() {
    array < HANDLE, 1forget_frforget_frame for (auto& e : events) {
auto ptp_event_set_test() {
    array<HANDLE, 10> events{};
    for (auto& e : events) {
        e = CreateEventEx(nullptr, nullptr, //
                          CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        _require_(e != NULL);
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
    SetEvent(ev);

    SleepEx(3, true); // give time to windows threads
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
        FAIL_WITH_MESSAGE(system_category().message(ec));
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
        FAIL_WITH_MESSAGE(system_category().message(ec));
    }
}
#elif __has_include(<CppUnitTest.h>)
class ptp_event_set : public TestClass<ptp_event_set> {
    TEST_METHOD(test_ptp_event_set) {
        ptp_event_set_test();
    }
};
#endif
