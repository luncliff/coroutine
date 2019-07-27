//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace coro;

auto invoke_and_suspend() -> frame {
    co_await suspend_always{};
    co_return;
};
auto coro_frame_first_suspend_test() {
    auto frame = invoke_and_suspend();

    // allow access to `coroutine_handle<void>`
    //  after first suspend(which can be `co_return`)
    coroutine_handle<void>& coro = frame;
    _require_(static_cast<bool>(coro)); // not null
    _require_(coro.done() == false);    // it is susepended !
    coro.destroy();

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_frame_first_suspend_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_frame_first_suspend : public TestClass<coro_frame_first_suspend> {
    TEST_METHOD(test_coro_frame_first_suspend) {
        coro_frame_first_suspend_test();
    }
};
#endif
