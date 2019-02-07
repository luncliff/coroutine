//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <coroutine/sequence.hpp>
#include <gsl/gsl>

TEST_CASE("async_generator", "[generic]")
{
    using namespace std::experimental;

    // for async generator,
    //  its coroutine frame must be alive for some case.
    return_frame fm{};
    auto ensure_destroy_frame = gsl::finally([=]() {
        auto coro = static_cast<coroutine_handle<void>>(fm);
        if (coro)
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
        REQUIRE_NOTHROW(fm = try_sequence(value));
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
        REQUIRE_NOTHROW(fm = try_sequence(value));
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
        REQUIRE_NOTHROW(fm = try_sequence(value));
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
        REQUIRE_NOTHROW(fm = try_sequence(value));
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

            REQUIRE_NOTHROW(fm = try_sequence(value));
        }
        REQUIRE(value == 666);
    }
}
