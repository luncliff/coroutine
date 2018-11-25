//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <thread>

#include <coroutine/sync.h>

TEST_CASE("SectionTest", "[thread]")
{
    // critical section
    section mtx{};

    constexpr auto max_repeat = 1'000;

    auto stress_on_lock = [&]() {
        auto repeat = max_repeat;
        while (repeat--)
        {
            std::lock_guard lck{mtx};

            using namespace std::literals;
            std::this_thread::yield();
            std::this_thread::sleep_for(1000ns);
        }
    };

    // trigger some interleaving...

    std::thread t1{stress_on_lock};
    std::thread t2{stress_on_lock};
    std::thread t3{stress_on_lock};
    std::thread t4{stress_on_lock};
    std::thread t5{stress_on_lock};
    std::thread t6{stress_on_lock};
    std::thread t7{stress_on_lock};

    REQUIRE_NOTHROW(t1.join());
    REQUIRE_NOTHROW(t2.join());
    REQUIRE_NOTHROW(t3.join());
    REQUIRE_NOTHROW(t4.join());
    REQUIRE_NOTHROW(t5.join());
    REQUIRE_NOTHROW(t6.join());
    REQUIRE_NOTHROW(t7.join());
}
