//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

#include "./test.h"
#include <catch.hpp>

// ensure successful write to channel
template<typename L>
auto write_to(channel<uint64_t, L>& ch, uint64_t value, bool ok = false)
    -> unplug
{
    ok = co_await ch.write(value);

    if (ok == false)
        // inserting fprintf makes the crash disappear.
        // finding the reason for the issue
        fprintf(stdout, "write_to %p %llx \n", &value, value);

    REQUIRE(ok);
}

// ensure successful read from channel
template<typename L>
auto read_from(channel<uint64_t, L>& ch, uint64_t& value, bool ok = false)
    -> unplug
{
    std::tie(value, ok) = co_await ch.read();
    REQUIRE(ok);
}

TEST_CASE("ChannelTest", "[channel]")
{
    // - Note
    //      Lockable without lock operation
    struct bypass_lock
    {
        bool try_lock() noexcept { return true; }
        void lock() noexcept {}
        void unlock() noexcept {}
    };

    SECTION("WriteRead")
    {
        uint64_t storage = 0;
        channel<uint64_t, bypass_lock> ch{};

        // Writer coroutine may suspend.(not-returned)
        for (uint64_t i = 0u; i < 3; ++i)
            write_to(ch, i);

        for (uint64_t i = 0u; i < 3; ++i)
        {
            // read to `storage`
            read_from(ch, storage);
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }

    SECTION("ReadWrite")
    {
        uint64_t storage = 0;
        channel<uint64_t, bypass_lock> ch{};

        // Reader coroutine may suspend.(not-returned)
        for (uint64_t i = 0u; i < 3; ++i)
            // read to `storage`
            read_from(ch, storage);

        for (uint64_t i = 0u; i < 3; ++i)
        {
            write_to(ch, i);
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }

    SECTION("SimpleRace")
    {
        // using 'guaranteed' lockable
        channel<uint64_t, std::mutex> ch{};
        uint32_t success = 0, failure = 0;

        switch_to background{};

        static constexpr size_t TryCount = 100'000;

        wait_group group{};
        group.add(2 * TryCount);

        auto send_with_callback = [&](auto& ch, auto value, auto fn) -> unplug {
            co_await background; // go to background

            const auto ok = co_await ch.write(value);
            fn(ok);
        };
        auto recv_with_callback = [&](auto& ch, auto fn) -> unplug {
            co_await background; // go to background

            // [ value, ok ]
            const auto tup = co_await ch.read();
            fn(std::get<1>(tup));
        };

        auto callback = [&](bool ok) {
            // increase counter according to operation result
            if (ok)
                success += 1;
            else
                failure += 1;

            // notify the end of coroutine
            group.done();
        };

        // Spawn coroutines
        uint64_t repeat = TryCount;
        while (repeat--)
        {
            recv_with_callback(ch, callback);
            send_with_callback(ch, repeat, callback);
        }

        // Wait for all coroutines...
        // !!! use should ensure there is no race for destroying channel !!!
        group.wait();

        REQUIRE(failure == 0);
        REQUIRE(success == 2 * TryCount);
    }
}
