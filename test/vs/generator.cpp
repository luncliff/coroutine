// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <coroutine/enumerable.hpp>
#include <coroutine/unplug.hpp>

using namespace std::literals;
using namespace std::experimental;

class GeneratorTest : public TestClass<GeneratorTest>
{
  public:
    TEST_METHOD(Accumulate)
    {
        auto values = []() -> enumerable<int> {
            for (int v = 0; v < 10; ++v)
                co_yield v;

            co_return;
        };

        auto g = values();
        Assert::IsTrue(std::accumulate(g.begin(), g.end(), 0) == 45);
    }

    TEST_METHOD(NoYield)
    {
        auto values = []() -> enumerable<int> {
            int v = 1111;

            if (false)
                co_yield v;

            co_return;
        };

        int storage = 2222;
        for (auto v : values())
            storage = v;

        Assert::IsTrue(storage == 2222);
    }
};
