//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

// if it doesn't need to care about a coroutine's life cycle, use
//  `no_return`. `co_return` with this type will destroy the frame
auto invoke_and_forget_frame() -> no_return {
    co_await suspend_never{};
    co_return;
};
auto coro_no_return_test() {
    try {
        invoke_and_forget_frame();
    } catch (const exception& ex) {
        FAIL_WITH_MESSAGE(string{ex.what()});
    }
    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class coro_no_return : public TestClass<coro_no_return> {
    TEST_METHOD(test_coro_no_return) {
        coro_no_return_test();
    }
};
#else
int main(int, char* []) {
    return coro_no_return_test();
}
#endif
