//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <coroutine/sequence.hpp>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

TEST_CASE("SequenceTest", "[syntax]")
{
    SECTION("no_result")
    {
        auto get_sequence = [](await_point& plug) -> sequence<int> {
            co_yield plug; // do nothing
        };
        auto try_sequence =
            [](await_point& plug, int& ref, auto async_gen) -> unplug {
            for
                co_await(int v : async_gen(plug)) ref = v;
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

        auto try_sequence =
            [](await_point& plug, int& ref, auto async_gen) -> unplug {
            for
                co_await(int v : async_gen(plug)) ref = v;
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

        auto try_sequence =
            [=](await_point& plug, int& ref, auto async_gen) -> unplug {
            for
                co_await(int v : async_gen(plug)) ref = v;
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
        auto get_sequence = [=](await_point& plug,
                                int& value) -> sequence<int> {
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

        auto try_sequence = [](await_point& plug,
                               int& value,
                               int& sum,
                               auto async_gen) -> unplug { //
            REQUIRE(sum == 0);
            auto s = async_gen(plug, value);
            REQUIRE(sum == 0);

            // for (auto it = co_await s.begin(); // begin
            //     it != s.end();                // check
            //     co_await++ it                 // advance
            //)
            for
                co_await(int value : s)
                {
                    // auto value = *it;
                    sum += value;
                }

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

    SECTION("interleaved-mt")
    {
        WARN("This test seems ok but need more verification under "
             "hard-racing");

        auto get_sequence = [](wait_group& wg, int& value) -> sequence<int> {
            co_yield switch_to{};

            co_yield value = 1;
            co_yield switch_to{};

            co_yield value = 2;
            co_yield value = 3;
            co_yield switch_to{};

            value = -3;
            co_yield switch_to{};

            co_yield value = 4;
            wg.done();
        };

        auto try_sequence = [](wait_group& wg, //
                               int& value,
                               int& sum,
                               auto async_gen) -> unplug {
            REQUIRE(sum == 0);
            auto s = async_gen(wg, value);
            REQUIRE(sum == 0);
            for
                co_await(int value : s) { sum += value; }

            REQUIRE(sum == 10);
            sum += 5;
            wg.done();
        };

        int value = 0;
        int sum = 0;
        wait_group wg{};

        wg.add(2);
        REQUIRE_NOTHROW(                               //
            try_sequence(wg, value, sum, get_sequence) //
        );
        wg.wait();
        REQUIRE(sum == 15);
    }
}
