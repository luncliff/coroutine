//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>

#include "test.h"
using namespace coro;

// if you doesn't need to care about a coroutine's life cycle,
// use `forget_frame`.
// the frame will be destroyed after `co_return`
auto invoke_and_forget_frame() -> forget_frame {
    co_await suspend_never{};
    co_return;
};
auto coro_forget_frame_with_await() {
    try {
        invoke_and_forget_frame();
    } catch (const exception& ex) {
        _println_(ex.what());
        return __LINE__;
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_forget_frame_with_await();
}

#elif __has_include(<CppUnitTest.h>)
class coro_forget_frame : public TestClass<coro_forget_frame> {
    TEST_METHOD(test_coro_forget_frame) {
        coro_forget_frame_test();
    }
};
#endif
