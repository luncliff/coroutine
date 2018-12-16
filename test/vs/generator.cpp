// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <array>
#include <numeric>

#include <coroutine/enumerable.hpp>
#include <coroutine/return.h>

using namespace std::literals;
using namespace std::experimental;

class generator_test : public TestClass<generator_test>
{
  public:
    TEST_METHOD(generator_never_yield)
    {
        auto generate_values = []() -> enumerable<uint16_t> {
            // co_return is necessary so compiler can notice that
            // this is a coroutine when there is no co_yield.
            co_return;
        };

        int trunc, count = 0;
        for (uint16_t v : generate_values())
        {
            trunc = v;
            count += 1;
        }

        Assert::IsTrue(count == 0);
    }

    TEST_METHOD(generator_yield_once)
    {
        auto generate_values = []() -> enumerable<int> {
            int value = 0;
            co_yield value;
            co_return;
        };

        int count = 0;
        for (int v : generate_values())
        {
            Assert::IsTrue(v == 0);
            count += 1;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(generator_for_accumulate)
    {
        auto generate_values = [](uint16_t n) -> enumerable<uint16_t> {
            co_yield n;
            while (--n)
                co_yield n;

            co_return;
        };

        auto g = generate_values(10);
        auto total = std::accumulate(g.begin(), g.end(), 0u);
        Assert::IsTrue(total == 55);
    }

    TEST_METHOD(generator_for_max_element)
    {
        std::array<uint16_t, 10> container{};
        uint16_t id = 15;
        for (auto& e : container)
            e = id--; // [15, 14, 13 ...
                      // so the first element will hold the largest number

        // since generator is not a container,
        //  using max_element (or min_element) function on it
        //  will return invalid iterator
        auto generate_values = [&]() -> enumerable<uint16_t> {
            for (auto e : container)
                co_yield e;

            co_return;
        };

        auto g = generate_values();
        auto it = std::max_element(g.begin(), g.end());

        // after iteration is finished (co_return),
        // the iterator will hold nullptr.
        Assert::IsTrue(it.operator->() == nullptr);

        // so referencing it will lead to access violation
        // Assert::IsTrue(*it == 15);
    }
};
