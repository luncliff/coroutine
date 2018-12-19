//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch.hpp>

#include <array>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

// ensure successful write to channel
template <typename L>
auto write_to(channel<uint64_t, L>& ch, uint64_t value, bool ok = false)
    -> unplug
{
    ok = co_await ch.write(value);
    if (ok == false)
    {
        // inserting fprintf makes the crash disappear.
        // fprintf(stdout, "write_to %p %llx \n", &value, value);

        // !!!!!
        // seems like optimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;
    }

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
    for (auto& n : nums)
        n = id++;

    // To prevent code duplication,
    // test codes are in 1 lambda function.
    // The only argument will be some unknown channel type
    auto test_operations = [&](auto& ch) {
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
    };

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
        using channel_without_lock_t = channel<uint64_t, bypass_lock>;

        channel_without_lock_t ch{};
        // test channel with bypass_lock
        test_operations(ch);
    }

    GIVEN("std::mutex")
    {
        using channel_with_lock_t = channel<uint64_t, std::mutex>;

        channel_with_lock_t ch{};
        // test channel with standard mutex
        test_operations(ch);
    }
}

TEST_CASE("channel race test", "[thread][channel]")
{
    // using some thread-safe lockable
    using channel_t = channel<uint64_t, section>;
    channel_t chan{};

    SECTION("ensure delivery for same read/write")
    {
        // Too much stress can lead to failure. In the case,
        // library will throw exceptions. Therefore, be cautious about
        // granularity of works in coroutine
        static constexpr size_t max_count = 2'000;
        uint32_t success = 0, failure = 0;

        wait_group group{};
        group.add(2 * max_count);

        auto send_with_callback
            = [&](channel_t& ch, uint64_t value, wait_group& wg) -> unplug {
            // go to background
            co_await switch_to{};

            // return false if the channel is destructed(closed)
            if (co_await ch.write(value))
                success += 1;
            else
                failure += 1;

            wg.done();
        };

        auto recv_with_callback = [&](channel_t& ch, wait_group& wg) -> unplug {
            // go to background
            co_await switch_to{};

            // return { ?, false } if the channel is destructed(closed)
            auto [value, ok] = co_await ch.read();
            if (ok)
                success += 1;
            else
                failure += 1;

            wg.done();
        };

        auto count = max_count;
        while (count--) // what if repeat value is used as an id
                        // of alive coroutine?
        {
            // Spawn coroutines

            recv_with_callback(chan, group);
            send_with_callback(chan, count, group);
        }

        // Wait for all coroutines...
        // !!! use should ensure there is no race for destroying channel !!!
        group.wait();

        // there is no failure for same read/write
        REQUIRE(failure == 0);

        // since we are using background threads with `switch_to`,
        // the race condition for `success` will lead to lessor than
        // number of spawned coroutines
        REQUIRE(success <= 2 * max_count);
    }
}
