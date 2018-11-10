//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch.hpp>
#include <coroutine/unplug.hpp>

TEST_CASE("UnplugTest", "[syntax]")
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

TEST_CASE("PlugTest", "[syntax]")
{
    SECTION("empty") { REQUIRE_NOTHROW(await_plug{}.resume()); }

    SECTION("plug_and_resume")
    {
        auto try_plugging = [](await_plug& p, int& status) -> unplug {
            status = 1;
            co_await std::experimental::suspend_never{};
            co_await p;
            status = 2;
            co_return;
        };

        int status = 0;
        await_plug p{};

        REQUIRE_NOTHROW(try_plugging(p, status));
        REQUIRE(status == 1);
        p.resume();
        REQUIRE(status == 2);
    }
}
