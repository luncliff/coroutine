//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto invoke_and_suspend_immediately() -> frame_t {
    co_await suspend_always{};
    co_return;
};

auto coro_preserve_frame_destroy_with_return_test() {
    auto frame = invoke_and_suspend_immediately();

    // coroutine_handle<void> is final_suspended after `co_return`.
    coroutine_handle<void>& coro = frame;

    assert(static_cast<bool>(coro)); // not null
    assert(coro.done() == false);    // it is susepended !

    // we don't care. destroy it
    coro.destroy();

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_preserve_frame_destroy_with_return_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_preserve_frame_destroy_with_return
    : public TestClass<coro_preserve_frame_destroy_with_return> {
    TEST_METHOD(test_coro_preserve_frame_destroy_with_return) {
        coro_preserve_frame_destroy_with_return_test();
    }
};
#endif
