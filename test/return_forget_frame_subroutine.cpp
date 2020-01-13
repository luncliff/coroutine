//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

// the type is default constructible
auto invoke_and_no_await() -> forget_frame {
    return {};
};

auto coro_forget_frame_without_await_test() -> int {
    try {
        invoke_and_no_await();
        return EXIT_SUCCESS;

    } catch (const exception& ex) {
        fputs(ex.what(), stderr);
        return __LINE__;
    }
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_forget_frame_without_await_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_forget_frame_without_await
    : public TestClass<coro_forget_frame_without_await> {
    TEST_METHOD(test_coro_forget_frame_without_await) {
        coro_forget_frame_without_await_test();
    }
};
#endif
