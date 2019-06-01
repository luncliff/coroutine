//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

auto coro_frame_empty_test() {
    frame fh{};
    auto coro = static_cast<coroutine_handle<void>>(fh);
    REQUIRE(coro.address() == nullptr);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_frame_empty_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_frame_empty : public TestClass<coro_frame_empty> {
    TEST_METHOD(test_coro_frame_empty) {
        coro_frame_empty_test();
    }
};
#endif
