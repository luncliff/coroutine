//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/frame.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <atomic>
#include <cstddef>

using namespace std;
using namespace std::experimental;

TEST_CASE("coroutine_handle", "[primitive][semantics]") {
    // helper object to use address
    std::byte blob[64]{};
    coroutine_handle<void> c1{}, c2{};
    // libc++ requires the parameter must be void* type
    void *p1 = blob + 2, *p2 = blob + 3;

    SECTION("move") {
        c1 = coroutine_handle<void>::from_address(p1);
        c2 = coroutine_handle<void>::from_address(p2);

        REQUIRE(c1.address() == p1);
        REQUIRE(c2.address() == p2);
        c2 = move(c1);
        // expect the address is moved to c2
        REQUIRE(c1.address() == p1);
        REQUIRE(c2.address() == p1);
    }

    SECTION("swap") {
        c1 = coroutine_handle<void>::from_address(p1);
        c2 = coroutine_handle<void>::from_address(p2);

        REQUIRE(c1.address() != nullptr);
        REQUIRE(c2.address() != nullptr);

        swap(c1, c2);
        REQUIRE(c1.address() == p2);
        REQUIRE(c2.address() == p1);
    }
}
