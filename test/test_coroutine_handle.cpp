//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

auto coroutine_handle_move_test() {
    coroutine_handle<void> c1{}, c2{};
    // libc++ requires the parameter must be void* type
    void* p1 = &c1;
    void* p2 = &c2;
    c1 = coroutine_handle<void>::from_address(p1);
    c2 = coroutine_handle<void>::from_address(p2);

    REQUIRE(c1.address() == p1);
    REQUIRE(c2.address() == p2);

    c2 = move(c1);
    // expect the address is moved to c2
    REQUIRE(c1.address() == p1);
    REQUIRE(c2.address() == p1);
}

auto coroutine_handle_swap_test() {
    coroutine_handle<void> c1{}, c2{};
    // libc++ requires the parameter must be void* type
    void* p1 = &c1;
    void* p2 = &c2;
    c1 = coroutine_handle<void>::from_address(p1);
    c2 = coroutine_handle<void>::from_address(p2);

    REQUIRE(c1.address() != nullptr);
    REQUIRE(c2.address() != nullptr);

    swap(c1, c2);
    REQUIRE(c1.address() == p2);
    REQUIRE(c2.address() == p1);
}

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("coroutine_handle move", "[coroutine_handle]") {
    coroutine_handle_move_test();
}
TEST_CASE("coroutine_handle swap", "[coroutine_handle]") {
    coroutine_handle_swap_test();
}

#elif __has_include(<CppUnitTest.h>)
class coroutine_handle_move : public TestClass<coroutine_handle_move> {
    TEST_METHOD(test_coroutine_handle_move) {
        coroutine_handle_move_test();
    }
};
class coroutine_handle_swap : public TestClass<coroutine_handle_swap> {
    TEST_METHOD(test_coroutine_handle_swap) {
        coroutine_handle_swap_test();
    }
};
#endif
