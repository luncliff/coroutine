//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <thread>

#include "../modules/stop_watch.hpp"
#include <coroutine/sync.h>

TEST_CASE("section", "[sync][thread]")
{
    // mutex for critical section
    section mtx{};

    constexpr auto max_repeat = 1'000;

    auto stress_on_lock = [&]() {
        auto repeat = max_repeat;
        while (repeat--)
        {
            std::lock_guard lck{mtx};

            using namespace std::literals;
            std::this_thread::yield();
        }
    };

    // threads will race on the mutex
    std::thread //
        t1{stress_on_lock},
        t2{stress_on_lock}, t3{stress_on_lock}, t4{stress_on_lock},
        t5{stress_on_lock}, t6{stress_on_lock}, t7{stress_on_lock};

    REQUIRE_NOTHROW(t1.join());
    REQUIRE_NOTHROW(t2.join());
    REQUIRE_NOTHROW(t3.join());
    REQUIRE_NOTHROW(t4.join());
    REQUIRE_NOTHROW(t5.join());
    REQUIRE_NOTHROW(t6.join());
    REQUIRE_NOTHROW(t7.join());
}

TEST_CASE("wait_group", "[sync][thread]")
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
        REQUIRE(wg.wait());
    }
}