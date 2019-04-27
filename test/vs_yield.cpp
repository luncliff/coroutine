//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/yield.hpp>

#include <array>
#include <numeric>

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace coro;

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
        auto generate_values = [&container]() -> enumerable<uint16_t> {
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
        auto try_sequence = [](int& ref) -> no_return {
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
        auto try_sequence = [](int& ref) -> no_return {
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
        frame sp{};

        auto example = [&sp]() -> sequence<int> {
            co_yield sp; // suspend
            co_return;
        };
        auto try_sequence = [&](int& ref) -> no_return {
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
        auto try_sequence = [example](int& ref) -> no_return {
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
        frame sp{};

        auto example = [&]() -> sequence<int> {
            int v{};
            co_yield v = 444;
            co_yield sp;
            co_yield v = 555;
            co_return;
        };

        // reference capture
        auto try_sequence = [&example](int& ref) -> no_return {
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
        frame sp{};
        {
            auto example = [&]() -> sequence<int> {
                int v{};
                co_yield v = 666;
                co_yield sp;
                co_yield v = 777;
            };
            auto try_sequence = [&example](int& ref) -> no_return {
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
