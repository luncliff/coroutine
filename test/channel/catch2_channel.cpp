//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <array>
#include <thread>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>
#include <coroutine/switch.h>

// ensure successful write to channel
template <typename L>
auto write_to(channel<uint64_t, L>& ch, uint64_t value, bool ok = false)
    -> unplug
{
    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like optimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;

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

SCENARIO("channel", "[generic][channel]")
{
    using namespace std;
    using namespace std::literals;

    GIVEN("channel with bypass lock")
    {
        // lockable without lock operation
        struct bypass_lock
        {
            bool try_lock() noexcept
            {
                return true;
            }
            void lock() noexcept
            {
                // do nothing since this is bypass lock
            }
            void unlock() noexcept
            {
                // it is not locked
            }
        };
        using channel_without_lock_t = channel<uint64_t, bypass_lock>;

        channel_without_lock_t ch{};
        uint64_t storage = 0;
        std::array<uint64_t, 3> nums = {1, 2, 3};

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

    GIVEN("channel with mutex")
    {
        using channel_with_lock_t = channel<uint64_t, std::mutex>;

        channel_with_lock_t ch{};
        uint64_t storage = 0;
        std::array<uint64_t, 3> nums = {1, 2, 3};

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

    GIVEN("background worker")
    {
        switch_to background{};
        auto& sched = background.scheduler();

        auto resume_tasks = [](scheduler_t* sc) {
            using namespace std::literals;

            while (sc->closed() == false)
                // for timeout, `wait` returns empty handle
                if (auto coro = sc->wait(1ms))
                    coro.resume();
        };

        std::thread w1{resume_tasks, std::addressof(sched)};
        std::thread w2{resume_tasks, std::addressof(sched)};

        // using some thread-safe lockable
        using channel_t = channel<uint64_t, section>;
        channel_t ch{};

        WHEN("same read/write operation")
        THEN("no operation leakage")
        {
            // Too much stress can lead to failure.
            // Library will throw exceptions.
            // Therefore, be cautious about granularity of works in coroutine
            static constexpr size_t max_count = 2'000;
            uint32_t success = 0, failure = 0;

            wait_group group{};
            group.add(2 * max_count);

            auto send_with_callback
                = [&](channel_t& ch, uint64_t value, wait_group& wg) -> unplug {
                co_await background; // go to background

                // return false if the channel is destructed(closed)
                if (co_await ch.write(value))
                    success += 1;
                else
                    failure += 1;

                wg.done();
            };

            auto recv_with_callback
                = [&](channel_t& ch, wait_group& wg) -> unplug {
                co_await background; // go to background

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
                recv_with_callback(ch, group);
                send_with_callback(ch, count, group);
            }

            // Wait for all coroutines...
            // !!! use should ensure there is no race for destroying channel !!!
            group.wait(30s);

            // there is no failure for same read/write
            REQUIRE(failure == 0);

            // since we are using background threads with `switch_to`,
            // the race condition for `success` will lead to lessor than
            // number of spawned coroutines
            REQUIRE(success > 0);
            REQUIRE(success <= 2 * max_count);
        }

        sched.close();
        REQUIRE_NOTHROW(w1.join(), w2.join());
    }
}
