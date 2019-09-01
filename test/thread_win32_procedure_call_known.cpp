//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto procedure_call_on_known_thread(HANDLE thread, DWORD tid, HANDLE finished)
    -> forget_frame {
    co_await procedure_call_on{thread};

    _require_(tid == GetCurrentThreadId());
    SetEvent(finished);
}

DWORD __stdcall stay_alertible(LPVOID param) {
    DWORD ec{};
    while (ec == 0) { // retry in case of timeout (test on CI may run slow...)
        ec = SleepEx(5000, true);
    }
    return ec;
}

auto win32_procedure_call_on_known_thread() {
    // 0: new thread
    // 1: event for the coroutine
    HANDLE handles[2]{};
    HANDLE& thread = handles[0];
    HANDLE& event = handles[1];

    event = CreateEvent(nullptr, false, false, nullptr);
    _require_(event != INVALID_HANDLE_VALUE);

    DWORD tid{};
    thread = CreateThread(nullptr, 2048, stay_alertible, nullptr, 0, &tid);
    _require_(thread != INVALID_HANDLE_VALUE);

    procedure_call_on_known_thread(thread, tid, event);

    constexpr bool wait_all = true;
    auto ec = WaitForMultipleObjectsEx(2, handles, wait_all, INFINITE, true);
    _require_(ec == WAIT_OBJECT_0 || ec == WAIT_IO_COMPLETION, //
              __FILE__, __LINE__);
    CloseHandle(event);

    DWORD retcode{};
    GetExitCodeThread(thread, &retcode);
    CloseHandle(thread);
    // we used QueueUserAPC so the return must be 'elapsed' milliseconds
    _require_(retcode > 0, __FILE__, __LINE__);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return win32_procedure_call_on_known_thread();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_win32_procedure_call_on_known
    : public TestClass<coro_win32_procedure_call_on_known> {
    TEST_METHOD(test_win32_procedure_call_on_known) {
        win32_procedure_call_on_known_thread();
    }
};
#endif
