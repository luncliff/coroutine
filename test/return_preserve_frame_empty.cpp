//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto coro_frame_empty_test() {
    preserve_frame frame{};

    assert(frame.address() == nullptr);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_frame_empty_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_frame_empty : public TestClass<coro_frame_empty> {
    TEST_METHOD(test_coro_frame_empty) {
        coro_frame_empty_test();
    }
};
#endif
