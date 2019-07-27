//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace coro;

auto coroutine_handle_swap_test() {
    coroutine_handle<void> c1{}, c2{};
    // libc++ _require_s the parameter must be void* type
    void* p1 = &c1;
    void* p2 = &c2;
    c1 = coroutine_handle<void>::from_address(p1);
    c2 = coroutine_handle<void>::from_address(p2);

    _require_(c1.address() != nullptr);
    _require_(c2.address() != nullptr);

    swap(c1, c2);
    _require_(c1.address() == p2);
    _require_(c2.address() == p1);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coroutine_handle_swap_test();
}

#elif __has_include(<CppUnitTest.h>)
class coroutine_handle_swap : public TestClass<coroutine_handle_swap> {
    TEST_METHOD(test_coroutine_handle_swap) {
        coroutine_handle_swap_test();
    }
};
#endif
