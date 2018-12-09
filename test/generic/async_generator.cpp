//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <coroutine/return.h>
#include <coroutine/sequence.hpp>
#include <coroutine/sync.h>

TEST_CASE("async_generator", "[generic]")
{
    // ....
    frame_holder h{};

    SECTION("return without yield")
    {
        auto example = []() -> sequence<int> {
            co_return; // do nothing
        };

        auto try_sequence = [=](int& ref) -> frame_holder {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
        };

        int value = 111;
        REQUIRE_NOTHROW(h = try_sequence(value));
        REQUIRE(value == 111);
    }

    SECTION("suspend and return")
    {
        suspend_hook sp{};

        auto example = [&]() -> sequence<int> {
            co_yield sp; // suspend
            co_return;
        };
        auto try_sequence = [&](int& ref) -> frame_holder {
            // clang-format off
            for co_await(int v : example()) 
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 222;
        REQUIRE_NOTHROW(h = try_sequence(value));
        REQUIRE(value == 222);
        REQUIRE_NOTHROW(sp.resume());
    }

    SECTION("yield once")
    {
        auto example = []() -> sequence<int> {
            int v = 333;
            co_yield v;
            co_return;
        };
        auto try_sequence = [=](int& ref) -> frame_holder {
            // clang-format off
            for co_await(int v : example()) 
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        REQUIRE_NOTHROW(h = try_sequence(value));
        REQUIRE(value == 333);
    }

    SECTION("yield suspend yield")
    {
        suspend_hook sp{};

        auto example = [&]() -> sequence<int> {
            int v{};
            co_yield v = 444;
            co_yield sp;
            co_yield v = 555;
            co_return;
        };
        auto try_sequence = [&](int& ref) -> frame_holder {
            // clang-format off
            for co_await(int v : example()) 
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        REQUIRE_NOTHROW(h = try_sequence(value));
        REQUIRE(value == 444);
        REQUIRE_NOTHROW(sp.resume());
        REQUIRE(value == 555);
    }

    SECTION("destroy in suspend")
    {
        int value = 0;
        suspend_hook sp{};

        {
            auto example = [&]() -> sequence<int> {
                int v{};
                co_yield v = 666;
                co_yield sp;
                co_yield v = 777;
            };
            auto try_sequence = [&](int& ref) -> frame_holder {
                // clang-format off
                for co_await(int v : example()) 
                    ref = v;
                // clang-format on
                co_return;
            };

            REQUIRE_NOTHROW(h = try_sequence(value));
        }
        REQUIRE(value == 666);
    }
}
