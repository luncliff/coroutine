//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

// if you doesn't need to care about a coroutine's life cycle,
// use `forget_frame`.
// the frame will be destroyed after `co_return`
auto invoke_and_forget_frame() -> forget_frame {
    co_await suspend_never{};
    co_return;
};

auto coro_forget_frame_with_await_test() -> int {
    try {
        invoke_and_forget_frame();
        return EXIT_SUCCESS;

    } catch (const exception& ex) {
        fputs(ex.what(), stderr);
        return __LINE__;
    }
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_forget_frame_with_await_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_forget_frame_with_await
    : public TestClass<coro_forget_frame_with_await> {
    TEST_METHOD(test_coro_forget_frame_with_await) {
        coro_forget_frame_with_await_test();
    }
};
#endif
