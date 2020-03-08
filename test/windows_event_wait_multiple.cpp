/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <atomic>
#include <cassert>
#include <iostream>

#include <gsl/gsl>

#include <coroutine/return.h>
#include <coroutine/windows.h>

using namespace std;
using namespace coro;

auto wait_an_event(set_or_cancel& token, atomic_flag& flag) -> frame_t;
auto set_after_sleep(HANDLE ev, uint32_t ms) -> frame_t;

int main(int, char*[]) {
    array<HANDLE, 10> events{};
    for (auto& e : events) {
        e = CreateEventEx(nullptr, nullptr, //
                          CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        assert(e != NULL);
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
    // issue:
    //	CI environment runs slowly, so too short timeout might fail ...
    //	wait for 300 ms
    constexpr bool wait_all = true;
    auto ec = WaitForMultipleObjectsEx((DWORD)events.max_size(), events.data(),
                                       wait_all, 300, true);
    assert(ec == WAIT_OBJECT_0);
    return EXIT_SUCCESS;
}

auto wait_an_event(set_or_cancel& evt, atomic_flag& flag) -> frame_t {
    // wait for set or cancel
    // `co_await` will forward `GetLastError` if canceled.
    if (DWORD ec = co_await evt) {
        cerr << system_category().message(ec) << endl;
        co_return;
    }
    flag.test_and_set();
}

auto set_after_sleep(HANDLE ev, uint32_t ms) -> frame_t {
    // move to background thread ...
    co_await continue_on_thread_pool{};
    SleepEx(ms, true);
    // if failed, print error message
    if (SetEvent(ev) == FALSE)
        cerr << system_category().message(GetLastError()) << endl;
}
