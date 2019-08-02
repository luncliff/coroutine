//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

auto invoke_and_suspend_immediately() -> preserve_frame {
    co_await suspend_always{};
    co_return;
};

auto coro_preserve_frame_destroy_with_return() {
    auto frame = invoke_and_suspend_immediately();

    // coroutine_handle<void> is final_suspended after `co_return`.
    coroutine_handle<void>& coro = frame;

    _require_(static_cast<bool>(coro)); // not null
    _require_(coro.done() == false);    // it is susepended !

    // we don't care. destroy it
    coro.destroy();

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_preserve_frame_destroy_with_return();
}

#elif __has_include(<CppUnitTest.h>)
class coro_frame_first_suspend : public TestClass<coro_frame_first_suspend> {
    TEST_METHOD(test_coro_frame_first_suspend) {
        coro_frame_first_suspend_test();
    }
};
#endif
