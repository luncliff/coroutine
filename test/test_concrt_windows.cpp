//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto thread_helper_test() {
    auto pid = GetCurrentProcessId();
    bool detected = false;
    // it's simple. find peer threads
    for (auto tid : get_threads(pid)) {
        if (tid == GetCurrentThreadId())
            continue;
        detected = true;
    }
    // since visual studio native test runs with multi-thread,
    // there must be a peer thread for this test process
    REQUIRE(detected);
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

void ptp_event_set_test() {
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
    SetEvent(ev);

    SleepEx(3, true); // give time to windows threads
    REQUIRE(flag.test_and_set());
}

void ptp_event_cancel_test() {
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
}

void ptp_event_wait_one_test() {
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
}

void ptp_event_wait_multiple_test() {
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
    for (auto e : events) {
        auto ms = rand() & 0b1111; // at most 16 ms
        set_after_sleep(e, ms);
    }

    SleepEx(3, true);
    // issue: CI environment runs slowly, so too short timeout might fail
    // wait for 300 ms
    auto ec = WaitForMultipleObjectsEx( //
        gsl::narrow_cast<DWORD>(events.max_size()), events.data(), true, 300,
        true);
    REQUIRE(ec == WAIT_OBJECT_0);
}

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("concrt thread api utility", "[concurrency]") {
    thread_helper_test();
}
TEST_CASE("concrt ptp_event set", "[event]") {
    ptp_event_set_test();
}
TEST_CASE("concrt ptp_event cancel", "[event]") {
    ptp_event_cancel_test();
}
TEST_CASE("concrt ptp_event wait one", "[event][concurrency]") {
    ptp_event_wait_one_test();
}
TEST_CASE("concrt ptp_event wait multiple", "[event][concurrency]") {
    ptp_event_wait_multiple_test();
}

#elif __has_include(<CppUnitTest.h>)
class win32_thread_helper : public TestClass<win32_thread_helper> {
    TEST_METHOD(test_win32_thread_helper) {
        thread_helper_test();
    }
};
class ptp_event_set : public TestClass<ptp_event_set> {
    TEST_METHOD(test_ptp_event_set) {
        ptp_event_set_test();
    }
};
class ptp_event_cancel : public TestClass<ptp_event_cancel> {
    TEST_METHOD(test_ptp_event_cancel) {
        ptp_event_cancel_test();
    }
};
class ptp_event_wait_one : public TestClass<ptp_event_wait_one> {
    TEST_METHOD(test_ptp_event_wait_one) {
        ptp_event_wait_one_test();
    }
};
class ptp_event_wait_multiple : public TestClass<ptp_event_wait_multiple> {
    TEST_METHOD(test_ptp_event_wait_multiple) {
        ptp_event_wait_multiple_test();
    }
};

#endif
