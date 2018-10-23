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
        {
            REQUIRE(v == 0);
        }
    }

    SECTION("sequence")
    {
        auto get_sequence = []() -> sequence<int> {
            int value = 0;
            puts("get_sequence.1");
            co_await std::experimental::suspend_never{};
            puts("get_sequence.2");
            value = 1;
            co_yield value;
            puts("get_sequence.3");
            co_await std::experimental::suspend_never{};
            puts("get_sequence.4");
            co_await std::experimental::suspend_never{};
            puts("get_sequence.5");
            value = 2;
            co_yield value;
            puts("get_sequence.6");
            value = 3;
            co_yield value;
            puts("get_sequence.7");
            co_await std::experimental::suspend_never{};
            puts("get_sequence.8");
            value = 4;
            co_yield value;
            puts("get_sequence.9");
            value = 5;
            co_return;
        };

        auto try_sequence = [=](int& sum) -> unplug {
            printf("try_sequence start\n");
            auto s = get_sequence();
            printf("try_sequence begin\n");
            for
                co_await(int value : s) //
                {
                    co_await std::experimental::suspend_never{};

                    printf("try_sequence %d\n", value);

                    sum += value;
                }
            printf("try_sequence end\n");
        };

        int sum1 = 0;
        REQUIRE_NOTHROW(try_sequence(sum1));
        REQUIRE(sum1 == 10);
    }
}
