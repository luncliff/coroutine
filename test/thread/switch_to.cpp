//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <thread>

#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

auto MoveOnce(wait_group& wg,
              thread_id_t& before,
              thread_id_t& after) noexcept(false) -> unplug
{
    switch_to back{}; // 0 means background thread

    before = current_thread_id();
    co_await back; // switch to designated thread

    after = current_thread_id();
    wg.done();
}

auto MoveToSpeicified( // switch twice...
    wait_group& wg,
    thread_id_t target_id) noexcept(false) -> unplug
{
    const auto start_id = current_thread_id();
    std::printf("MoveToSpecifid %lx \n", start_id);

    switch_to back{}, front{static_cast<uint64_t>(target_id)};

    co_await back;

    std::printf("co_await back \n");

    REQUIRE(start_id != current_thread_id());

    co_await front;
    std::printf("co_await front \n");

    REQUIRE(target_id == current_thread_id());

    wg.done();
}

TEST_CASE("SwitchToThreadTest", "[thread][messaging]")
{
    SECTION("ToUnknown")
    {
        wait_group wg{};
        thread_id_t id1, id2;

        wg.add(1);
        MoveOnce(wg, id1, id2);
        wg.wait();

        // get different thread id
        REQUIRE(id1 != id2);
    }

    SECTION("ToForeground")
    {
        using namespace std::experimental;

        coroutine_handle<void> coro{};
        wait_group wg{};
        thread_id_t id = current_thread_id();

        wg.add(1);
        MoveToSpeicified(wg, id);

        while (peek_switched(coro) == false)
        {
            // retry until we fetch the coroutine
            using namespace std::literals;
            std::this_thread::sleep_for(5s);
            continue;
        }
        std::printf("peek_switched %p \n", coro.address());

        coro.resume();

        wg.wait();
    }

    // SECTION("ForkJoin")
    // {
    //     // ...
    // }
}
