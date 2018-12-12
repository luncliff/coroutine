//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

#include <mutex>

#include "./test.h"
#include <catch.hpp>

// ensure successful write to channel
template <typename L>
auto write_to(channel<uint64_t, L>& ch, uint64_t value, bool ok = false)
    -> unplug
{
    ok = co_await ch.write(value);
    REQUIRE(ok);
}

// ensure successful read from channel
template <typename L>
auto read_from(channel<uint64_t, L>& ch, uint64_t& value, bool ok = false)
    -> unplug
{
    std::tie(value, ok) = co_await ch.read();
    REQUIRE(ok);
}

SCENARIO("basic channel operations", "[generic][channel]")
{
    uint64_t storage = 0;
    std::array<uint64_t, 3> nums{};

    uint64_t id = 1;
    for (auto& i : nums)
        i = id++;

    GIVEN("bypass lock")
    {
        // - Note
        //      Lockable without lock operation
        struct bypass_lock
        {
            bool try_lock() noexcept
            {
                return true;
            }
            void lock() noexcept
            {
            }
            void unlock() noexcept
            {
            }
        };

        channel<uint64_t, bypass_lock> ch{};

        THEN("write before read")
        {
            // Writer coroutine may suspend.(not-returned)
            for (auto i : nums)
                write_to(ch, i);

            for (auto i : nums)
            {
                // read to `storage`
                read_from(ch, storage);
                REQUIRE(storage == i); // stored value is same with sent value
            }
        }

        THEN("read before write")
        {
            // Writer coroutine may suspend.(not-returned)
            for (auto i : nums)
                write_to(ch, i);

            for (auto i : nums)
            {
                // read to `storage`
                read_from(ch, storage);
                REQUIRE(storage == i); // stored value is same with sent value
            }
        }
    }

    GIVEN("std::mutex")
    {
        channel<uint64_t, std::mutex> ch{};

        THEN("write before read")
        {
            // Writer coroutine may suspend.(not-returned)
            for (auto i : nums)
                write_to(ch, i);

            for (auto i : nums)
            {
                // read to `storage`
                read_from(ch, storage);
                REQUIRE(storage == i); // stored value is same with sent value
            }
        }

        THEN("read before write")
        {
            // Writer coroutine may suspend.(not-returned)
            for (auto i : nums)
                write_to(ch, i);

            for (auto i : nums)
            {
                // read to `storage`
                read_from(ch, storage);
                REQUIRE(storage == i); // stored value is same with sent value
            }
        }
    }
}

TEST_CASE("channel race test", "[thread][channel]")
{
    // using some thread-safe lockable
    using channel_t = channel<uint64_t, section>;
    channel_t chan{};

    SECTION("ensure delivery for same read/write")
    {
        static constexpr size_t max_count = 2'000;

        uint32_t success = 0, failure = 0;

        wait_group group{};
        group.add(2 * max_count);

        auto send_with_callback
            = [&](channel_t& ch, uint64_t value, wait_group& wg) -> unplug {
            // go to background
            co_await switch_to{};

            if (co_await ch.write(value))
                success += 1;
            else
                failure += 1;

            wg.done();
        };

        auto recv_with_callback = [&](channel_t& ch, wait_group& wg) -> unplug {
            // go to background
            co_await switch_to{};

            auto [value, ok] = co_await ch.read();
            if (ok)
                success += 1;
            else
                failure += 1;

            wg.done();
        };

        // Spawn coroutines
        uint64_t count = max_count;
        while (count--) // what if repeat value is used as an id
                        // of alive coroutine?
        {
            recv_with_callback(chan, group);
            send_with_callback(chan, count, group);
        }

        // Wait for all coroutines...
        // !!! use should ensure there is no race for destroying channel !!!
        group.wait();

        REQUIRE(failure == 0);
        REQUIRE(success <= 2 * max_count);
    }
}
