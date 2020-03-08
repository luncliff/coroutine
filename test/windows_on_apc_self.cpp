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

auto call_on_known_thread(HANDLE thread, HANDLE event) -> frame_t {
    co_await continue_on_apc{thread};
    if (SetEvent(event) == FALSE)
        cerr << system_category().message(GetLastError()) << endl;
}

int main(int, char*[]) {
    HANDLE event = CreateEvent(nullptr, false, false, nullptr);
    assert(event != INVALID_HANDLE_VALUE);
    auto on_return_1 = gsl::finally([event]() { CloseHandle(event); });

    HANDLE worker = GetCurrentThread();

    call_on_known_thread(worker, event);

    auto ec = WaitForSingleObjectEx(event, INFINITE, true);
    // expect the wait is cancelled by APC (WAIT_IO_COMPLETION)
    assert(ec == WAIT_OBJECT_0 || ec == WAIT_IO_COMPLETION);
    return EXIT_SUCCESS;
}
