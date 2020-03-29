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

auto procedure_call_on_known_thread(HANDLE thread, HANDLE event) -> frame_t {
    co_await continue_on_apc{thread};
    if (SetEvent(event) == FALSE)
        cerr << system_category().message(GetLastError()) << endl;
}

DWORD WINAPI wait_in_sleep(LPVOID) {
    SleepEx(1000, true);
    return GetLastError();
}

int main(int, char*[]) {
    HANDLE event = CreateEvent(nullptr, false, false, nullptr);
    assert(event != INVALID_HANDLE_VALUE);
    auto on_return_1 = gsl::finally([event]() { CloseHandle(event); });

    DWORD worker_id{};
    HANDLE worker = CreateThread(nullptr, 0, //
                                 wait_in_sleep, nullptr, 0, &worker_id);
    assert(worker != 0);
    auto on_return_2 = gsl::finally([worker]() { CloseHandle(worker); });
    SleepEx(500, true);

    procedure_call_on_known_thread(worker, event);

    HANDLE handles[2] = {event, worker};
    auto ec = WaitForMultipleObjectsEx(2, handles, TRUE, INFINITE, true);
    // expect the wait is cancelled by APC (WAIT_IO_COMPLETION)
    assert(ec == WAIT_OBJECT_0 || ec == WAIT_IO_COMPLETION);

    DWORD retcode{};
    GetExitCodeThread(worker, &retcode);
    // we used QueueUserAPC so the return can be 'elapsed' milliseconds
    // allow zero for the timeout
    assert(retcode >= 0);
    return EXIT_SUCCESS;
}
