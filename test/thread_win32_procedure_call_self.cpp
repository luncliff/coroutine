//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto procedure_call_self(HANDLE& thread, HANDLE finished) -> forget_frame {
    co_await procedure_call_on{thread};

    thread = GetCurrentThread();
    SetEvent(finished);
}

auto win32_procedure_call_on_self() {
    auto efinish = CreateEvent(nullptr, false, false, nullptr);
    auto thread = GetCurrentThread();

    procedure_call_self(thread, efinish);

    auto ec = WaitForSingleObjectEx(efinish, INFINITE, true);
    CloseHandle(efinish);

    // expect the wait is cancelled by APC
    _require_(ec == WAIT_IO_COMPLETION);
    _require_(thread == GetCurrentThread());

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return win32_procedure_call_on_self();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_win32_procedure_call_on_self
    : public TestClass<coro_win32_procedure_call_on_self> {
    TEST_METHOD(test_win32_procedure_call_on_self) {
        win32_procedure_call_on_self();
    }
};
#endif
