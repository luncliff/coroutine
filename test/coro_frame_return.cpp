//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

// when the coroutine frame destuction need to be controlled manually,
//  just return `frame`. The type `frame` implements `return_void`,
//  `suspend_never`(initial), `suspend_always`(final)
auto invoke_and_return() -> frame {
    co_await suspend_never{};
    co_return; // only void return
};
auto coro_frame_return_test() {
    auto frame = invoke_and_return();

    // allow access to `coroutine_handle<void>`
    //  after first suspend(which can be `co_return`)
    coroutine_handle<void>& coro = frame;
    REQUIRE(static_cast<bool>(coro)); // not null
    REQUIRE(coro.done());             // expect final suspended
    coro.destroy();

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_frame_return_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_frame_return : public TestClass<coro_frame_return> {
    TEST_METHOD(test_coro_frame_return) {
        coro_frame_return_test();
    }
};
#endif
