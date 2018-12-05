//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

#include <mutex>

#include "./test.h"
#include <catch.hpp>

// ensure successful write to channel
template <typename L>
auto write_to(channel<uint64_t, L>& ch, uint64_t value, bool ok = false)
    -> unplug
{
    ok = co_await ch.write(value);

    if (ok == false)
        // inserting fprintf makes the crash disappear.
        // finding the reason for the issue
        fprintf(stdout, "write_to %p \n", &value);

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

TEST_CASE("ChannelTest", "[generic][channel]")
{
    SECTION("WriteRead")
    {
        uint64_t storage = 0;
        std::array<uint64_t, 3> nums{};

        uint64_t id = 1;
        for (auto& i : nums)
            i = id++;

        SECTION("bypass_lock")
        {
            channel<uint64_t, bypass_lock> ch{};

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
        SECTION("std::mutex")
        {
            channel<uint64_t, std::mutex> ch{};

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

    SECTION("ReadWrite")
    {
        uint64_t storage = 0;
        std::array<uint64_t, 3> nums{};

        uint64_t id = 1;
        for (auto& i : nums)
            i = id++;

        SECTION("bypass_lock")
        {
            channel<uint64_t, bypass_lock> ch{};

            // Reader coroutine may suspend.(not-returned)
            // for (auto i : nums)
            for (auto i = 0u; i < 3; ++i)
                // read to `storage`
                read_from(ch, storage);

            for (auto i : nums)
            {
                write_to(ch, i);
                REQUIRE(storage == i); // stored value is same with sent value
            }
        }
        SECTION("std::mutex")
        {
            channel<uint64_t, std::mutex> ch{};

            // Reader coroutine may suspend.(not-returned)
            for (auto i = 0u; i < 3; ++i)
                // read to `storage`
                read_from(ch, storage);

            for (auto i : nums)
            {
                write_to(ch, i);
                REQUIRE(storage == i); // stored value is same with sent value
            }
        }
    }

    SECTION("EnsureDelivery")
    {
        try
        {
            // using 'guaranteed' lockable
            channel<uint64_t, section> ch{};
            uint32_t success = 0, failure = 0;

            // go to background (id: 0)
            static constexpr auto back_id = 0;
            static constexpr size_t TryCount = 6'000;

            wait_group group{};
            group.add(2 * TryCount);

            auto send_with_callback = [&]( //
                                          auto value, auto fn) -> unplug {
                bool ok = false;
                co_await switch_to{back_id};

                // try
                // {
                ok = co_await ch.write(value);
                // }
                // catch (const std::exception& e)
                // {
                //     ::fputs(e.what(), stderr);
                //     // ignore exception. consider as failure
                //     ok = false;
                // }
                fn(ok);
            };

            auto recv_with_callback = [&](auto fn) -> unplug {
                bool ok = false;
                co_await switch_to{back_id};

                // try
                // {
                // [ value, ok ]
                const auto tup = co_await ch.read();
                ok = std::get<1>(tup);
                // }
                // catch (const std::exception& e)
                // {
                //     ::fputs(e.what(), stderr);
                //     // ignore exception. consider as failure
                //     ok = false;
                // }
                fn(ok);
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
            while (repeat--) // what if repeat value is used as an id
                             // of alive coroutine?
            {
                recv_with_callback(callback);
                send_with_callback(repeat, callback);
            }

            // Wait for all coroutines...
            // !!! use should ensure there is no race for destroying channel !!!
            group.wait();

            REQUIRE(failure == 0);
            REQUIRE(success <= 2 * TryCount);
        }
        catch (std::system_error& e)
        {
            ::fputs(e.what(), stderr);

            FAIL(e.what());
        }
    }
}
