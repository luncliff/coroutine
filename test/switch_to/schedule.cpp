#include <catch2/catch.hpp>

#include "switch.h"

TEST_CASE("switch", "[stage]")
{
    auto example_code = [](switch_to_2& sw, uint32_t& status) -> unplug {
        status = 2;
        co_await sw;
        status = 3;
    };

    switch_to_2 sw{};
    uint32_t status{};

    example_code(sw, status);
    REQUIRE(status == 2);

    for (auto task : sw.schedule())
    {
        REQUIRE(task);
        REQUIRE_NOTHROW(task.resume());
    }

    REQUIRE(status == 3);
}
