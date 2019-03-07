//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/enumerable.hpp>
#include <gsl/gsl>

TEST_CASE("generator", "[generic]")
{
    using namespace std;

    SECTION("yield never")
    {
        auto example = []() -> enumerable<uint16_t> {
            // co_return is necessary so compiler can notice that
            // this is a coroutine when there is no co_yield.
            co_return;
        };

        auto count = 0u;
        for (auto v : example())
            count += 1;
        REQUIRE(count == 0);
    }

    SECTION("yield once")
    {
        auto example = []() -> enumerable<int> {
            int value = 0;
            co_yield value;
        };

        auto count = 0u;
        SECTION("range for")
        {
            for (auto v : example())
            {
                REQUIRE(v == 0);
                count += 1;
            }
        }
        SECTION("iterator")
        {
            auto g = example();
            auto b = g.begin();
            auto e = g.end();
            for (auto it = b; it != e; ++it)
            {
                REQUIRE(*it == 0);
                count += 1;
            }
        }
        SECTION("after move")
        {
            auto g = example();
            auto m = std::move(g);
            // g lost its handle. so it is not iterable anymore
            REQUIRE(g.begin() == g.end());

            for (auto v : m)
            {
                REQUIRE(v == 0);
                count += 1;
            }
        }
        REQUIRE(count > 0);
    }

    SECTION("accumulate")
    {
        auto example = [](uint16_t n) -> enumerable<uint16_t> {
            co_yield n;
            while (--n)
                co_yield n;

            co_return;
        };

        auto g = example(10);
        auto total = accumulate(g.begin(), g.end(), 0u);
        REQUIRE(total == 55);
    }

    SECTION("max_element")
    {
        // since generator is not a container,
        //  using max_element (or min_element) function on it
        //  will return invalid iterator
        auto example = [](auto& storage) -> enumerable<uint16_t> {
            for (auto& e : storage)
                co_yield e;

            co_return;
        };

        auto id = 15u;
        array<uint16_t, 10> storage{};
        for (auto& e : storage)
            e = id--; // [15, 14, 13 ...
                      // so the first element will hold the largest number

        auto g = example(storage);
        auto it = max_element(g.begin(), g.end());

        // after iteration is finished (co_return),
        // the iterator will hold nullptr.
        REQUIRE(it.operator->() == nullptr);

        // so referencing it will lead to access violation
        // REQUIRE(*it == 15);
    }
}
