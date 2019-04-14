//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include "./channel_test.h"

void test_require_true(bool cond)
{
    REQUIRE(cond);
}

TEST_CASE("channel without lock", "[generic][channel]")
{
    using element_t = int;
    using channel_without_lock_t = channel<element_t, bypass_lock>;

    channel_without_lock_t ch{};
    element_t storage = 0;

    const auto list = {1, 2, 3};

    SECTION("write before read")
    {
        for (auto i : list)
        {
            write_to(ch, i);       // Writer coroutine will suspend
            REQUIRE(storage != i); // so no write occurs
        }

        for (auto i : list)
        {
            read_from(ch, storage); // read to `storage`
            REQUIRE(storage == i);  // stored value is same with sent value
        }
    }
    SECTION("read before write")
    {
        for (auto i : list)
        {
            read_from(ch, storage); // Reader coroutine will suspend
            REQUIRE(storage != i);  // so no read occurs
        }
        for (auto i : list)
        {
            write_to(ch, i);       // writer will send a value
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }
}

TEST_CASE("channel with mutex", "[generic][channel]")
{
    using element_t = int;
    using channel_with_lock_t = channel<element_t, mutex>;

    channel_with_lock_t ch{};
    element_t storage = 0;

    const auto list = {1, 2, 3};

    SECTION("write before read")
    {
        for (auto i : list)
        {
            write_to(ch, i);       // Writer coroutine will suspend
            REQUIRE(storage != i); // so no write occurs
        }

        for (auto i : list)
        {
            read_from(ch, storage); // read to `storage`
            REQUIRE(storage == i);  // stored value is same with sent value
        }
    }
    SECTION("read before write")
    {
        for (auto i : list)
        {
            read_from(ch, storage); // Reader coroutine will suspend
            REQUIRE(storage != i);  // so no read occurs
        }
        for (auto i : list)
        {
            write_to(ch, i);       // writer will send a value
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }
}

TEST_CASE("channel close", "[generic][channel]")
{
    using namespace std;
    using element_t = uint64_t;
    using channel_without_lock_t = channel<element_t, bypass_lock>;

    auto ch = make_unique<channel_without_lock_t>();
    bool ok = true;

    SECTION("write return false after close")
    {
        auto coro_write = [&ok](auto& ch, auto value) -> return_frame {
            ok = co_await ch.write(value);
        };

        // coroutine will suspend and wait in the channel
        auto h = coro_write(*ch, element_t{});
        {
            auto truncator = std::move(ch); // if channel is destroyed ...
        }
        REQUIRE(ch.get() == nullptr);

        auto coro = h.get();
        REQUIRE(coro.done()); // coroutine is in done state
        coro.destroy();       // destroy to prevent leak
    }

    SECTION("read return false after close")
    {
        auto coro_read = [&ok](auto& ch, auto& value) -> return_frame {
            tie(value, ok) = co_await ch.read();
        };

        auto item = element_t{};
        // coroutine will suspend and wait in the channel
        auto h = coro_read(*ch, item);
        {
            auto truncator = std::move(ch); // if channel is destroyed ...
        }
        REQUIRE(ch.get() == nullptr);

        auto coro = h.get();
        REQUIRE(coro.done()); // coroutine is in done state
        coro.destroy();       // destroy to prevent leak
    }

    REQUIRE(ok == false); // and channel is returned false
}

TEST_CASE("channel select", "[generic][channel]")
{
    // it's singe thread, so mutex for channels doesn't have to be real lockable
    using u32_chan_t = channel<uint32_t, bypass_lock>;
    using i32_chan_t = channel<int32_t, bypass_lock>;

    SECTION("match one")
    {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        write_to(ch1, 17u);
        select(ch2,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), int32_t>);
                   FAIL("select on empty channel must bypass");
               },
               ch1,
               [](auto v) -> return_ignore {
                   static_assert(is_same_v<decltype(v), uint32_t>);
                   REQUIRE(v == 17u);

                   co_await suspend_never{};
               });
    }

    SECTION("no match")
    {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        select(ch1,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), uint32_t>);
                   FAIL("select on empty channel must bypass");
               },
               ch2,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), int32_t>);
                   FAIL("select on empty channel must bypass");
               });
    }

    SECTION("match both")
    {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        write_to(ch1, 17u);
        write_to(ch2, 15);

        select(ch2, [](auto v) { REQUIRE(v == 15); }, //
               ch1, [](auto v) { REQUIRE(v == 17u); } //
        );
    }
};
