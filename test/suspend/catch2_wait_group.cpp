//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <thread>

#include "stop_watch.hpp"
#include <coroutine/sync.h>

TEST_CASE("wait_group", "[sync]")
{
    using namespace std::chrono;
    using namespace std::literals;
    wait_group wg{};

    SECTION("timeout")
    {
        wg.add(1);

        stop_watch<steady_clock> watch{};

        watch.reset();
        REQUIRE_FALSE(wg.wait(900ms));

        CAPTURE(watch.pick<nanoseconds>().count());
        REQUIRE(watch.pick<milliseconds>() >= 899ms); // allow misc error
    }

    SECTION("already done")
    {
        wg.add(1);
        wg.done();

        stop_watch<steady_clock> watch{};

        watch.reset();
        REQUIRE(wg.wait(1ms) == true);
        REQUIRE(watch.pick<milliseconds>() <= 1ms);
    }

    SECTION("multiple wait doesn't throw")
    {
        wg.add(1);
        wg.done();

        stop_watch<steady_clock> watch{};

        watch.reset();
        REQUIRE(wg.wait(1ms) == true);
        REQUIRE(watch.pick<milliseconds>() <= 1ms);

        // in the case. it must return immediately
        watch.reset();
        REQUIRE(wg.wait(1ms));
    }
}