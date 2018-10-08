// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <magic/coroutine.hpp>
#include <magic/plugin.hpp>

TEST_CASE("LambdaTest")
{
    auto try_coroutine = []() -> magic::unplug {
        co_await std::experimental::suspend_never{};
        co_return;
    };
    REQUIRE_NOTHROW(try_coroutine());

    auto try_generator = []() -> std::experimental::generator<int> {
        int value = 0;
        co_yield value;
        co_return;
    };
    // try_generator();
}
