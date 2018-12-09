// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <coroutine/return.h>
#include <coroutine/sequence.hpp>

using namespace std::literals;
using namespace std::experimental;

class AsyncGeneratorTest : public TestClass<AsyncGeneratorTest>
{
    TEST_METHOD(CheckLeak) // just repeat a lot
    {
        auto get_sequence = [](int value = rand()) -> sequence<int> {
            // yield random
            co_yield value;
        };
        auto try_sequence = [=](int& ref) -> unplug {
            /* clang-format off */
            for co_await(int v : get_sequence())
                ref = v;
            /* clang-format on*/
        };

        constexpr auto max_repeat = 100'000;
        int dummy{};

        size_t repeat = max_repeat;
        while (--repeat)
            try_sequence(dummy);

        Assert::IsTrue(repeat == 0);
    }

    TEST_METHOD(YieldThenAwait)
    {
        auto get_sequence = [&](suspend_hook& p) -> sequence<int> {
            int value = 137;
            co_yield value;
            co_yield p;
        };
        auto try_sequence = [=](suspend_hook& p, int& ref) -> unplug {
            /* clang-format off */
            for co_await(int v : get_sequence(p)) 
                ref = v;
            /* clang-format on */
        };

        suspend_hook p{};
        int value = 0;
        try_sequence(p, value);
        p.resume();
        Assert::IsTrue(value == 137);
    }

    TEST_METHOD(AwaitThenYield)
    {
        auto get_sequence = [&](suspend_hook& p) -> sequence<int> {
            int value = 131;
            co_yield p;
            co_yield value;
        };
        auto try_sequence = [=](suspend_hook& p, int& ref) -> unplug {
            /* clang-format off */
            for co_await(int v : get_sequence(p)) 
                ref = v;
            /* clang-format on */
        };

        suspend_hook p{};
        int value = 0;
        try_sequence(p, value);
        p.resume();
        Assert::IsTrue(value == 131);
    }

    TEST_METHOD(BreakAfterOne)
    {
        auto get_sequence = []() -> sequence<int> {
            int value{};
            co_yield value = 7;
        };
        auto try_sequence = [=](int& ref) -> unplug {
            /* clang-format off */
            for co_await(int v : get_sequence())
            {
                ref = v;
                break;
            }
            /* clang-format on */
        };

        int value = 0;
        try_sequence(value);
        Assert::IsTrue(value == 7);
    }

    TEST_METHOD(MultipleAwait)
    {
        auto get_sequence = [&](suspend_hook& p, int& value) -> sequence<int> {
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

        auto try_sequence
            = [=](suspend_hook& p, int& sum, int& value) -> unplug {
            Assert::IsTrue(sum == 0);
            auto s = get_sequence(p, value);
            Assert::IsTrue(sum == 0);

            /* clang-format off */

            // for (auto it = co_await s.begin(); // begin
            //     it != s.end();                // check
            //     co_await++ it                 // advance
            //)
            for co_await(int v : s)
            {
                // auto value = *it;
                sum += v;
            }
            /* clang-format on */

            Assert::IsTrue(sum == 10);
            sum += 5;
        };

        suspend_hook p{};
        int sum = 0, value = 0;

        try_sequence(p, sum, value);
        Assert::IsTrue(value == 0);

        p.resume();
        Assert::IsTrue(value == 1);

        p.resume();
        Assert::IsTrue(value == 3);

        p.resume();
        Assert::IsTrue(value == -3);

        p.resume();
        Assert::IsTrue(value == 4);
        Assert::IsTrue(sum == 15);
    }
};
