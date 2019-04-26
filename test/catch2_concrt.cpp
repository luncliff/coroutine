//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>
#include <coroutine/concrt.h>

#include <array>
#include <functional>
#include <future>

using namespace std;

TEST_CASE("latch", "[concurrency][thread]")
{
    using wait_group = concrt::latch;

    SECTION("ready after wait")
    {
        constexpr auto num_of_work = 10u;
        array<future<void>, num_of_work> contexts{};

        wait_group group{num_of_work};

        // fork - join
        for (auto& fut : contexts)
            fut = std::async(
                launch::async,
                [](wait_group* group) -> void { group->count_down(); },
                addressof(group));
        for (auto& fut : contexts)
            fut.wait();

        group.wait();
        REQUIRE(group.is_ready());
    }

    SECTION("count down and wait")
    {
        wait_group group{1};
        REQUIRE_FALSE(group.is_ready());

        group.count_down_and_wait();
        REQUIRE(group.is_ready());
    }

    SECTION("throws for negative")
    {
        SECTION("from positive")
        {
            wait_group group{1};
            REQUIRE_THROWS(group.count_down(4));
        }
        SECTION("from zero")
        {
            wait_group group{0};
            REQUIRE_THROWS(group.count_down(2));
        }
    }
}
