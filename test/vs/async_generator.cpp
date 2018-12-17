// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/sequence.hpp>

#include <Windows.h>
#include <sdkddkver.h>

#include <CppUnitTest.h>
#include <TlHelp32.h>
#include <threadpoolapiset.h>

using namespace std::literals;
using namespace std::experimental;

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

class async_generator_test : public TestClass<async_generator_test>
{
  public:
    TEST_METHOD(async_generator_ensure_no_leak) // just repeat a lot
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

    TEST_METHOD(async_generator_return_without_yield)
    {
        auto example = []() -> sequence<int> {
            co_return; // do nothing
        };

        auto try_sequence = [=](int& ref) -> unplug {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
        };

        int value = 111;
        try_sequence(value);
        Assert::IsTrue(value == 111);
    }

    TEST_METHOD(async_generator_suspend_then_return)
    {
        suspend_hook sp{};

        auto example = [&]() -> sequence<int> {
            co_yield sp; // suspend
            co_return;
        };
        auto try_sequence = [&](int& ref) -> unplug {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 222;
        try_sequence(value);
        Assert::IsTrue(value == 222);
        sp.resume();
    }
    TEST_METHOD(async_generator_yield_once)
    {
        auto example = []() -> sequence<int> {
            int v = 333;
            co_yield v;
            co_return;
        };
        auto try_sequence = [=](int& ref) -> unplug {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        try_sequence(value);
        Assert::IsTrue(value == 333);
    }

    TEST_METHOD(async_generator_yield_suspend_yield)
    {
        suspend_hook sp{};

        auto example = [&]() -> sequence<int> {
            int v{};
            co_yield v = 444;
            co_yield sp;
            co_yield v = 555;
            co_return;
        };
        auto try_sequence = [&](int& ref) -> unplug {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        try_sequence(value);
        Assert::IsTrue(value == 444);
        sp.resume();
        Assert::IsTrue(value == 555);
    }

    TEST_METHOD(async_generator_destroy_in_suspend)
    {
        int value = 0;
        suspend_hook sp{};
        {
            auto example = [&]() -> sequence<int> {
                int v{};
                co_yield v = 666;
                co_yield sp;
                co_yield v = 777;
            };
            auto try_sequence = [&](int& ref) -> unplug {
                // clang-format off
                for co_await(int v : example())
                    ref = v;
                // clang-format on
                co_return;
            };

            try_sequence(value);
        }
        Assert::IsTrue(value == 666);
    }
};
