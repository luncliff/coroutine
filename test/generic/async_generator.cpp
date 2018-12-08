//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <coroutine/sequence.hpp>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

TEST_CASE("async_generator", "[generic]")
{
    SECTION("no_result-1")
    {
        auto get_sequence = [](await_point& plug) -> sequence<int> {
            co_yield plug; // do nothing
        };
        auto try_sequence
            = [](await_point& plug, int& ref, auto async_gen) -> unplug {
            /* clang-format off */
            for co_await(int v : async_gen(plug)) 
              ref = v;
            /* clang-format on */

            ref = -2;
        };

        await_point p{};
        int value = 0;

        REQUIRE_NOTHROW(                         //
            try_sequence(p, value, get_sequence) //
        );
        p.resume();
        REQUIRE(value == -2);
    }

    SECTION("one_result-case1")
    {
        auto get_sequence = [](await_point& plug) -> sequence<int> {
            int v = 137;
            co_yield v;
            co_yield plug;
        };

        auto try_sequence
            = [](await_point& plug, int& ref, auto async_gen) -> unplug {
            /* clang-format off */
            for co_await(int v : async_gen(plug)) 
                ref = v;
            /* clang-format on */
        };

        await_point p{};
        int value = 0;

        REQUIRE_NOTHROW(                         //
            try_sequence(p, value, get_sequence) //
        );
        p.resume();
        REQUIRE(value == 137);
    }

    SECTION("one_result-case2")
    {
        auto get_sequence = [](await_point& plug) -> sequence<int> {
            int v = 131;
            co_yield plug;
            co_yield v;
        };

        auto try_sequence
            = [=](await_point& plug, int& ref, auto async_gen) -> unplug {
            /* clang-format off */
            for co_await(int v : async_gen(plug)) 
                ref = v;
            /* clang-format on */
        };

        await_point p{};
        int value = 0;

        REQUIRE_NOTHROW(                         //
            try_sequence(p, value, get_sequence) //
        );
        p.resume();
        REQUIRE(value == 131);
    }

    SECTION("interleaved")
    {
        auto get_sequence
            = [=](await_point& plug, int& value) -> sequence<int> {
            co_yield plug;

            co_yield value = 1;
            co_yield plug;

            co_yield value = 2;
            co_yield value = 3;
            co_yield plug;

            value = -3;
            co_yield plug;

            co_yield value = 4;
        };

        auto try_sequence = [](await_point& plug, int& value, int& sum,
                               auto async_gen) -> unplug { //
            REQUIRE(sum == 0);
            auto s = async_gen(plug, value);
            REQUIRE(sum == 0);

            // for (auto it = co_await s.begin(); // begin
            //     it != s.end();                // check
            //     co_await++ it                 // advance
            //)

            /* clang-format off */
            for co_await(int value : s)
            {
                // auto value = *it;
                sum += value;
            }
            /* clang-format on */

            REQUIRE(sum == 10);
            sum += 5;
        };

        await_point p{};
        int value = 0;
        int sum = 0;

        REQUIRE_NOTHROW(                              //
            try_sequence(p, value, sum, get_sequence) //
        );
        REQUIRE(value == 0);

        p.resume();
        REQUIRE(value == 1);

        p.resume();
        REQUIRE(value == 3);

        p.resume();
        REQUIRE(value == -3);

        p.resume();
        REQUIRE(value == 4);
        REQUIRE(sum == 15);
    }
}
