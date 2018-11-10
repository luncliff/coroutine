// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Reference
//      -
//      https://docs.microsoft.com/en-us/windows/desktop/ProcThread/using-the-thread-pool-functions
//
// ---------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <CppUnitTest.h>
#include <Windows.h>
#include <sdkddkver.h>
#include <threadpoolapiset.h>

#include <array>

#include <coroutine/sequence.hpp>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;
using namespace std::experimental;

TEST_CLASS(SequenceTest) {

    TEST_METHOD(CheckSpeedAndLeak) // just repeat a lot
    {
        auto get_sequence = [](int value = rand()) -> sequence<int> {
            co_yield value;
        };
        auto try_sequence = [=](int& ref) -> unplug {
            for
                co_await(int v : get_sequence()) ref = v;
        };

        constexpr auto max_repeat = 100'000;
        int dummy{};
        unsigned repeat = max_repeat;
        while (--repeat)
            try_sequence(dummy);

        Assert::IsTrue(repeat == 0);
    }

    TEST_METHOD(YieldThenAwait)
    {
        auto get_sequence = [&](await_plug& p) -> sequence<int> {
            int value = 137;
            co_yield value;
            co_yield p;
        };
        auto try_sequence = [=](await_plug& p, int& ref) -> unplug {
            for
                co_await(int v : get_sequence(p)) ref = v;
        };

        await_plug p{};
        int value = 0;
        try_sequence(p, value);
        p.resume();
        Assert::IsTrue(value == 137);
    }

    TEST_METHOD(AwaitThenYield)
    {
        auto get_sequence = [&](await_plug& p) -> sequence<int> {
            int value = 131;
            co_yield p;
            co_yield value;
        };
        auto try_sequence = [=](await_plug& p, int& ref) -> unplug {
            for
                co_await(int v : get_sequence(p)) ref = v;
        };

        await_plug p{};
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
            //co_yield value = 9;
        };
        auto try_sequence = [=](int& ref) -> unplug {
            for
                co_await(int v : get_sequence())
            {
                ref = v;
                break;
            }
        };

        int value = 0;
        try_sequence(value);
        Assert::IsTrue(value == 7);
    }

    TEST_METHOD(Interleaved)
    {
        auto get_sequence = [&](await_plug& p, int& value) -> sequence<int> {
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

        auto try_sequence = [=](await_plug& p, int& sum, int& value) -> unplug {
            Assert::IsTrue(sum == 0);
            auto s = get_sequence(p, value);
            Assert::IsTrue(sum == 0);

            // for (auto it = co_await s.begin(); // begin
            //     it != s.end();                // check
            //     co_await++ it                 // advance
            //)
            for
                co_await(int v : s)
            {
                // auto value = *it;
                sum += v;
            }

            Assert::IsTrue(sum == 10);
            sum += 5;
        };

        await_plug p{};
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
}
;
