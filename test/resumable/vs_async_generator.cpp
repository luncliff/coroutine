// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/suspend.h>
#include <coroutine/sequence.hpp>

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class async_generator_test : public TestClass<async_generator_test>
{
  public:
    static auto return_random_int(int value = rand()) -> sequence<int>
    {
        co_yield value; // random
        co_return;
    };

    TEST_METHOD(async_generator_ensure_no_leak) // just repeat a lot
    {
        auto try_sequence = [](int& ref) -> return_ignore {
            /* clang-format off */
            for co_await(int v : return_random_int())
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

    static auto return_nothing() -> sequence<int>
    {
        co_return; // do nothing
    };

    TEST_METHOD(async_generator_return_without_yield)
    {
        auto try_sequence = [](int& ref) -> return_ignore {
            // clang-format off
            for co_await(int v : return_nothing())
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

        auto example = [&sp]() -> sequence<int> {
            co_yield sp; // suspend
            co_return;
        };
        auto try_sequence = [&](int& ref) -> return_ignore {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 222;
        try_sequence(value);
        Assert::IsTrue(value == 222);

        coroutine_handle<void> coro = sp;
        coro.resume();
    }

    TEST_METHOD(async_generator_yield_once)
    {
        auto example = []() -> sequence<int> {
            int v = 333;
            co_yield v;
            co_return;
        };

        // copy capture
        auto try_sequence = [example](int& ref) -> return_ignore {
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

        // reference capture
        auto try_sequence = [&example](int& ref) -> return_ignore {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        try_sequence(value);
        Assert::IsTrue(value == 444);
        coroutine_handle<void> coro = sp;
        coro.resume();
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
            auto try_sequence = [&example](int& ref) -> return_ignore {
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
