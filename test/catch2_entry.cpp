//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <coroutine/frame.h>
#include <cstddef>

using namespace std;
using namespace std::experimental;

TEST_CASE("coroutine_handle", "[semantics]")
{
    // helper object to use address
    std::byte blob[64]{};

    coroutine_handle<void> c1{}, c2{};

    SECTION("move")
    {
        void* ptr = blob + 1; // some address
        c1 = coroutine_handle<void>::from_address(ptr);

        REQUIRE(c1.address() != nullptr);
        REQUIRE(c2.address() == nullptr);
        c2 = move(c1);
        // expect the address is moved to c2
        REQUIRE(c1.address() != nullptr);
        REQUIRE(c2.address() != nullptr);
    }

    SECTION("swap")
    {
        c1 = coroutine_handle<void>::from_address(blob + 1);
        c2 = coroutine_handle<void>::from_address(blob + 7);

        REQUIRE(c1.address() != nullptr);
        REQUIRE(c2.address() != nullptr);

        swap(c1, c2);
        REQUIRE(c1.address() == blob + 7);
        REQUIRE(c2.address() == blob + 1);
    }
}