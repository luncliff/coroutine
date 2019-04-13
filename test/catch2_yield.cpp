//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <coroutine/sequence.hpp>
#include <coroutine/suspend.h>
#include <gsl/gsl>

using namespace std;
using namespace std::experimental;
using namespace coro;

TEST_CASE("generator", "[generic]")
{
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

TEST_CASE("async_generator", "[generic]")
{
    using namespace std::experimental;

    // for async generator,
    //  its coroutine frame must be alive for some case.
    return_frame holder{};
    auto ensure_destroy_frame = gsl::finally([=]() {
        if (auto coro = holder.get())
            coro.destroy();
    });

    SECTION("return without yield")
    {
        auto example = []() -> sequence<int> {
            co_return; // do nothing
        };

        auto try_sequence = [=](int& ref) -> return_frame {
            // clang-format off
            for co_await(int v : example())
                ref = v;
            // clang-format on
        };

        int value = 111;
        REQUIRE_NOTHROW(holder = try_sequence(value));
        REQUIRE(value == 111);
    }

    SECTION("suspend and return")
    {
        suspend_hook sp{};

        auto example = [&]() -> sequence<int> {
            co_yield sp; // suspend
            co_return;
        };
        auto try_sequence = [&](int& ref) -> return_frame {
            // clang-format off
            for co_await(int v : example()) 
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 222;
        REQUIRE_NOTHROW(holder = try_sequence(value));
        REQUIRE(value == 222);

        auto coro = static_cast<coroutine_handle<void>>(sp);
        REQUIRE_NOTHROW(coro.resume());
    }

    SECTION("yield once")
    {
        auto example = []() -> sequence<int> {
            int v = 333;
            co_yield v;
            co_return;
        };
        auto try_sequence = [=](int& ref) -> return_frame {
            // clang-format off
            for co_await(int v : example()) 
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        REQUIRE_NOTHROW(holder = try_sequence(value));
        REQUIRE(value == 333);
    }

    SECTION("yield suspend yield")
    {
        suspend_hook sp{};

        auto example = [&]() -> sequence<int> {
            int v{};
            co_yield v = 444;
            co_yield sp;
            co_yield v = 555;
            co_return;
        };
        auto try_sequence = [&](int& ref) -> return_frame {
            // clang-format off
            for co_await(int v : example()) 
                ref = v;
            // clang-format on
            co_return;
        };

        int value = 0;
        REQUIRE_NOTHROW(holder = try_sequence(value));
        REQUIRE(value == 444);

        auto coro = static_cast<coroutine_handle<void>>(sp);
        REQUIRE_NOTHROW(coro.resume());
        REQUIRE(value == 555);
    }

    SECTION("destroy in suspend")
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
            auto try_sequence = [&](int& ref) -> return_frame {
                // clang-format off
                for co_await(int v : example()) 
                    ref = v;
                // clang-format on
                co_return;
            };

            REQUIRE_NOTHROW(holder = try_sequence(value));
        }
        REQUIRE(value == 666);
    }
}
