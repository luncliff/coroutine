//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/channel.hpp>
#include <coroutine/concrt.h>
#include <coroutine/return.h>

#include <array>
#include <future>
#include <thread>

using namespace std;
using namespace std::experimental;
using namespace coro;

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

// ensure successful write to channel
template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> return_ignore
{
    using namespace std;

    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like clang optimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;

    REQUIRE(ok);
}

// ensure successful read from channel
template <typename E, typename L>
auto read_from(channel<E, L>& ch, E& value, bool ok = false) -> return_ignore
{
    using namespace std;

    tie(value, ok) = co_await ch.read();
    REQUIRE(ok);
}

template <typename T, typename M, typename CountType>
auto write_and_count(channel<T, M>& ch, T value, //
                     CountType& success_count, CountType& failure_count)
    -> return_ignore
{
    bool ok = co_await ch.write(value);
    // ... ??? ...  // channel address is strange ...
    if (ok)
        success_count += 1;
    else
        failure_count += 1;
}

template <typename T, typename M, typename CountType>
auto read_and_count(channel<T, M>& ch, T& ref, //
                    CountType& success_count, CountType& failure_count)
    -> return_ignore
{
    auto [value, ok] = co_await ch.read();
    if (ok == false)
    {
        failure_count += 1;
        co_return;
    }
    ref = value;
    success_count += 1;
}

TEST_CASE("channel without lock", "[generic][channel]")
{
    using value_type = int;
    using channel_without_lock_t = channel<value_type, bypass_lock>;

    channel_without_lock_t ch{};
    value_type storage = 0;

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
    using value_type = int;
    using channel_with_lock_t = channel<value_type, mutex>;

    channel_with_lock_t ch{};
    value_type storage = 0;

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
    using value_type = uint64_t;
    using channel_without_lock_t = channel<value_type, bypass_lock>;

    auto ch = make_unique<channel_without_lock_t>();
    bool ok = true;

    SECTION("write return false after close")
    {
        auto coro_write = [&ok](auto& ch, auto value) -> return_frame {
            ok = co_await ch.write(value);
        };

        // coroutine will suspend and wait in the channel
        auto h = coro_write(*ch, value_type{});
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

        auto item = value_type{};
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

TEST_CASE("channel race", "[generic][channel]")
{
    using channel_type = channel<uint64_t, mutex>;
    using value_type = typename channel_type::value_type;

    struct context final
    {
        uint32_t success, failure;
        concrt::latch wg;
        channel_type ch;

        explicit context(uint32_t count) : ch{}, success{}, failure{}, wg{count}
        {
        }
        ~context() noexcept(false) = default;

        void write()
        {
            write_and_count(ch, value_type{}, success, failure);
            wg.count_down();
        }
        void read()
        {
            value_type value{};
            read_and_count(ch, value, success, failure);
            wg.count_down();
        }
    };

    SECTION("channel under race")
    {
        auto num_of_work = 1000u;

        context ctx{2 * num_of_work};

        auto futures = make_unique<future<void>[]>(num_of_work * 2);
        // simple fork-join
        for (auto i = 0u; i < num_of_work; ++i)
        {
            futures[2 * i + 0] = async(launch::async, //
                                       [](context* tc) {
                                           if (tc)
                                               tc->read();
                                       },
                                       &ctx);
            futures[2 * i + 1] = async(launch::async, //
                                       [](context* tc) {
                                           if (tc)
                                               tc->write();
                                       },
                                       &ctx);
        }
        ctx.wg.wait();

        std::for_each(futures.get(), futures.get() + num_of_work * 2,
                      [](future<void>& f) { f.get(); });

        // for same read/write operation,
        //  channel guarantees all reader/writer will be executed.
        REQUIRE(ctx.failure == 0);

        // however, the mutex in the channel is for matching of the coroutines.
        // so the counter in the context will be raced
        REQUIRE(ctx.success > 0);
        REQUIRE(ctx.success <= 2 * num_of_work);
    }
};
