//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <coroutine/unplug.hpp>

TEST_CASE("unplug", "[generic]")
{
    SECTION("default_use")
    {
        auto try_coroutine = []() -> unplug {
            co_await std::experimental::suspend_never{};
            co_return;
        };
        REQUIRE_NOTHROW(try_coroutine());
    }
}

TEST_CASE("await_point", "[generic]")
{
    SECTION("empty")
    {
        // if empty, do nothing
        REQUIRE_NOTHROW(await_point{}.resume());
    }

    SECTION("plug_and_resume")
    {
        int status = 0;
        {
            auto try_plugging = [=](await_point& p, int& status) -> unplug {
                auto a = defer([&]() {
                    // ensure final action
                    status = 3;
                });
                status = 1;
                co_await std::experimental::suspend_never{};
                co_await p;
                status = 2;
                co_await p;
                co_return;
            };

            await_point p{};

            REQUIRE_NOTHROW(try_plugging(p, status));
            REQUIRE(status == 1);
            p.resume();
            REQUIRE(status == 2);
            p.resume();
        }
        REQUIRE(status == 3);
    }
}
