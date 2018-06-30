// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <magic/coroutine.hpp>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

auto bypass() -> magic::unplug
{
    co_await std::experimental::suspend_never{};
    co_return;
}

TEST_CASE("SampleTest")
{
    bypass();
}
