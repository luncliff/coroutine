//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch.hpp>

#include <numeric>

#include <coroutine/enumerable.hpp>

TEST_CASE("GeneratorTest", "[syntax]")
{
    SECTION("yield_never")
    {
        auto generate_values = []() -> enumerable<uint16_t> { co_return; };

        int trunc, count = 0;
        for (uint16_t v : generate_values())
        {
            trunc = v;
            count += 1;
        }

        REQUIRE(count == 0);
    }

    SECTION("yield_once")
    {
        auto generate_values = []() -> enumerable<int> {
            int value = 0;
            co_yield value;
            co_return;
        };

        int count = 0;
        for (int v : generate_values())
        {
            REQUIRE(v == 0);
            count += 1;
        }
        REQUIRE(count > 0);
    }

    SECTION("accumulate")
    {
        auto generate_values = [](uint16_t n) -> enumerable<uint16_t> {
            co_yield n;
            while (--n)
                co_yield n;

            co_return;
        };

        auto g = generate_values(10);
        auto total = std::accumulate(g.begin(), g.end(), 0u);
        REQUIRE(total == 55);
    }
}
