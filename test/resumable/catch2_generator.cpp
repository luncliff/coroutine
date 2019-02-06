//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <array>
#include <numeric>

#include <coroutine/enumerable.hpp>

TEST_CASE("generator", "[generic]")
{
    SECTION("yield never")
    {
        auto try_enumerable = []() -> enumerable<uint16_t> {
            // co_return is necessary so compiler can notice that
            // this is a coroutine when there is no co_yield.
            co_return;
        };

        int trunc, count = 0;
        for (uint16_t v : try_enumerable())
        {
            trunc = v;
            count += 1;
        }

        REQUIRE(count == 0);
    }

    SECTION("yield once")
    {
        auto try_enumerable = []() -> enumerable<int> {
            int value = 0;
            co_yield value;
            co_return;
        };

        int count = 0;
        for (int v : try_enumerable())
        {
            REQUIRE(v == 0);
            count += 1;
        }
        REQUIRE(count > 0);
    }

    SECTION("accumulate")
    {
        auto try_enumerable = [](uint16_t n) -> enumerable<uint16_t> {
            co_yield n;
            while (--n)
                co_yield n;

            co_return;
        };

        auto g = try_enumerable(10);
        auto total = std::accumulate(g.begin(), g.end(), 0u);
        REQUIRE(total == 55);
    }

    SECTION("max_element")
    {
        std::array<uint16_t, 10> container{};
        uint16_t id = 15;
        for (auto& e : container)
            e = id--; // [15, 14, 13 ...
                      // so the first element will hold the largest number

        // since generator is not a container,
        //  using max_element (or min_element) function on it
        //  will return invalid iterator
        auto try_enumerable = [&]() -> enumerable<uint16_t> {
            for (auto e : container)
                co_yield e;

            co_return;
        };

        auto g = try_enumerable();
        auto it = std::max_element(g.begin(), g.end());

        // after iteration is finished (co_return),
        // the iterator will hold nullptr.
        REQUIRE(it.operator->() == nullptr);

        // so referencing it will lead to access violation
        // REQUIRE(*it == 15);
    }
}
