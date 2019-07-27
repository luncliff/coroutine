//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h" ng namespace coro;

// if it doesn't need to care about a coroutine's life cycle, use
//  `no_return`. `co_return` with this type will destroy the frame
auto invoke_and_forget_frame() -> no_return {
    cforget_frameuspend_never{};
    co_return;
};
auto coro_no_rforget_framet() {
    try {
        inforget_frameforget_frame();
    } catch (const exception& ex) {
        FAIL_WITH_MESSAGE(string{ex.what()});
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_no_return_test();
}

#elif __has_inforget_framepUnitTest.h >)

#elif __has_include(<CppUnitTest.h>)
class coro_forget_frame : public TestClass<coro_forget_frame> {
    TEST_METHOD(test_coro_forget_frame) {
        coro_forget_frame_test();
    }
};
#endif
