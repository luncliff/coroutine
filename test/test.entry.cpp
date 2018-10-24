// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <coroutine/sequence.hpp>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

TEST_CASE("ExpressionTest", "[syntax]")
{
    SECTION("unplug")
    {
        auto try_coroutine = []() -> unplug {
            co_await std::experimental::suspend_never{};
            co_return;
        };
        REQUIRE_NOTHROW(try_coroutine());
    }

    SECTION("generator")
    {
        auto try_generator = []() -> std::experimental::generator<int> {
            int value = 0;
            co_yield value;
            co_return;
        };

        for (auto v : try_generator())
            REQUIRE(v == 0);
    }

    SECTION("plug")
    {
        SECTION("empty") { REQUIRE_NOTHROW(plug{}.resume()); }

        SECTION("plugged")
        {
            int status = 0;
            plug routine{};

            auto try_plugging = [&](plug& p) -> unplug {
                status = 1;
                co_await p;
                status = 2;
                co_return;
            };

            REQUIRE_NOTHROW(try_plugging(routine));
            REQUIRE(status == 1);
            routine.resume();
            REQUIRE(status == 2);
        }
    }

    SECTION("sequence")
    {
        plug p{};

        SECTION("no_result")
        {
            auto get_sequence = [&](plug& p) -> sequence<int> { co_yield p; };

            auto try_sequence = [=](plug& p, int& ref) -> unplug {
                for
                    co_await(int v : get_sequence(p)) ref = v;

                ref = -2;
            };
            int value = 0;
            REQUIRE_NOTHROW(try_sequence(p, value));
            p.resume();
            REQUIRE(value == -2);
        }

        SECTION("one_result-case1")
        {
            auto get_sequence = [&](plug& p) -> sequence<int> {
                int value = 137;
                co_yield value;
                co_yield p;
            };

            auto try_sequence = [=](plug& p, int& ref) -> unplug {
                for
                    co_await(int v : get_sequence(p)) ref = v;
            };
            int value = 0;
            REQUIRE_NOTHROW(try_sequence(p, value));
            p.resume();
            REQUIRE(value == 137);
        }

        SECTION("one_result-case2")
        {
            auto get_sequence = [&](plug& p) -> sequence<int> {
                int value = 131;
                co_yield p;
                co_yield value;
            };

            auto try_sequence = [=](plug& p, int& ref) -> unplug {
                for
                    co_await(int v : get_sequence(p)) ref = v;
            };
            int value = 0;
            REQUIRE_NOTHROW(try_sequence(p, value));
            p.resume();
            REQUIRE(value == 131);
        }

        SECTION("interleaved")
        {
            int value = 0;
            auto get_sequence = [&](plug& p) -> sequence<int> {
                co_yield p;

                co_yield value = 1;
                co_yield p;

                co_yield value = 2;
                co_yield value = 3;
                co_yield p;

                value = -3;
                co_yield p;

                co_yield value = 4;
            };

            auto try_sequence = [=](plug& p, int& sum) -> unplug {
                REQUIRE(sum == 0);
                auto s = get_sequence(p);
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

            int sum = 0;
            REQUIRE_NOTHROW(try_sequence(p, sum));
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

            wait_group wg{};
            wg.add(2);

            int value = 0;
            auto get_sequence = [&]() -> sequence<int> {
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

            auto try_sequence = [&, get_sequence](int& sum) -> unplug {
                REQUIRE(sum == 0);
                auto s = get_sequence();
                REQUIRE(sum == 0);
                for
                    co_await(int value : s) { sum += value; }

                REQUIRE(sum == 10);
                sum += 5;
                wg.done();
            };

            int sum = 0;
            REQUIRE_NOTHROW(try_sequence(sum));
            wg.wait();

            REQUIRE(sum == 15);
        }

        // nothing will happen...
        REQUIRE_NOTHROW(p.resume());
    }
}
