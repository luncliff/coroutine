// 

#include <experimental/coroutine>
#include <magic/coroutine.hpp>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>


auto bypass() -> magic::unplug
{
    co_await std::experimental::suspend_never{};
    co_return;
}

TEST_CASE("Hello")
{
    bypass();
}
