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

auto bypass() -> magic::unplug
{
    co_await std::experimental::suspend_never{};
    co_return;
}

#if __APPLE__

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("Sample")
{
    bypass();
}

#elif __linux__ || __unix__

int main(int argc, char *argv[])
{
    bypass();
    return 0;
}

#endif