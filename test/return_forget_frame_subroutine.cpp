//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

// the type is default constructible
auto invoke_and_no_await() -> forget_frame {
    return {};
};

auto coro_forget_frame_without_await() {
    try {

        invoke_and_no_await();

    } catch (const exception& ex) {
        _println_(ex.what());
        return __LINE__;
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_forget_frame_without_await();
}

#elif __has_include(<CppUnitTest.h>)
class coro_forget_frame : public TestClass<coro_forget_frame> {
    TEST_METHOD(test_coro_forget_frame) {
        coro_forget_frame_test();
    }
};
#endif
