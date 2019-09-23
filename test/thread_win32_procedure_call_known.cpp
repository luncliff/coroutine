//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/thread.h>

#include <sstream>
#include <thread>

#include "test.h"
using namespace std;
using namespace coro;

auto procedure_call_on_known_thread(HANDLE thread, DWORD tid, HANDLE finished)
    -> forget_frame {
    co_await procedure_call_on{thread};

    _require_(tid == GetCurrentThreadId());
    SetEvent(finished);
}

void print_debug(const char* label, DWORD ec, size_t line) {
    std::stringstream sout{};
    sout << line << '\t' << label << '\t' << ec << endl;
    _println_(sout.str().c_str());
}

DWORD __stdcall stay_alertible(LPVOID) {
    DWORD retcode{};
    // give enough timeout in case of timeout (test on CI may run slow...)
    for (auto i = 0; i < 5; ++i) {
        auto retcode = SleepEx(1000, true);
        if (retcode == 0) // timeout
            continue;
        break;
    }
    print_debug("SleepEx", retcode, __LINE__);
    return retcode;
}

auto win32_procedure_call_on_known_thread() {
    HANDLE handles[2]{};
    HANDLE& worker = handles[0]; // 0: worker thread
    HANDLE& event = handles[1];  // 1: event for the coroutine

    event = CreateEvent(nullptr, false, false, nullptr);
    _require_(event != INVALID_HANDLE_VALUE);

    DWORD tid{};
    worker = CreateThread(nullptr, 2048, stay_alertible, nullptr, 0, &tid);
    _require_(worker != INVALID_HANDLE_VALUE);

    procedure_call_on_known_thread(worker, tid, event);

    constexpr bool wait_all = true;
    auto ec = WaitForMultipleObjectsEx(2, handles, wait_all, INFINITE, true);
    print_debug("WaitForMultipleObjectsEx", ec, __LINE__);
    _require_(ec == WAIT_OBJECT_0 || ec == WAIT_IO_COMPLETION, //
              __FILE__, __LINE__);
    CloseHandle(event);

    DWORD retcode{};
    GetExitCodeThread(worker, &retcode);
    print_debug("GetExitCodeThread", retcode, __LINE__);
    CloseHandle(worker);
    // we used QueueUserAPC so the return can be 'elapsed' milliseconds
    // allow zero for the timeout
    _require_(retcode >= 0, __FILE__, __LINE__);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
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
