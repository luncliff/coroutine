//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

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
    REQUIRE(static_cast<bool>(coro)); // not null
    REQUIRE(coro.done() == false);    // it is susepended !
    coro.destroy();

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return coro_frame_first_suspend_test();
}
#endif
