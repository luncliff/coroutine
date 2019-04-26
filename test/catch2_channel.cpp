//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/channel.hpp>
#include <coroutine/concrt.h>
#include <coroutine/suspend.h>

#include <gsl/gsl>

#include <array>
#include <future>

using namespace std;
using namespace experimental;
using namespace coro;


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
    using channel_without_lock_t = channel<value_type>;

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
    using channel_without_lock_t = channel<value_type>;

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
            auto truncator = move(ch); // if channel is destroyed ...
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
            auto truncator = move(ch); // if channel is destroyed ...
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
    using u32_chan_t = channel<uint32_t>;
    using i32_chan_t = channel<int32_t>;

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

class background final : public suspend_never
{
    std::future<void> fut{};
    auto request_async_resume(void* ptr) noexcept(false)
    {
        fut = std::async([=]() {
            if (auto coro = coroutine_handle<void>::from_address(ptr))
                coro.resume();
        });
    }

  public:
    void await_suspend(coroutine_handle<void> coro)
    {
        this->request_async_resume(coro.address());
    }
};

#if !defined(_WINDOWS)
using CRITICAL_SECTION = pthread_rwlock_t;
#endif

// standard lockable concept with win32 criticial section
class section final
{
    CRITICAL_SECTION cs;

  public:
    section() noexcept(false);
    ~section() noexcept;
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

    bool try_lock() noexcept;
    void lock() noexcept(false);
    void unlock() noexcept(false);
};

TEST_CASE("channel race", "[generic][channel]")
{
    using wait_group = concrt::latch;
    using value_type = uint64_t;
    using channel_type = channel<value_type, section>;

    SECTION("no leack under race")
    {
        static constexpr size_t max_try_count = 6'000;
        uint32_t success{}, failure{};
        wait_group group{2 * max_try_count};

        auto send_with_callback
            = [&](channel_type& ch, value_type value) -> return_ignore {
            co_await background{};

            auto w = co_await ch.write(value);
            w ? success += 1 : failure += 1;
            group.count_down();
        };
        auto recv_with_callback = [&](channel_type& ch) -> return_ignore {
            co_await background{};

            auto [value, r] = co_await ch.read();
            r ? success += 1 : failure += 1;
            group.count_down();
        };

        channel_type ch{};

        // Spawn coroutines
        uint64_t repeat = max_try_count;
        while (repeat--)
        {
            recv_with_callback(ch);
            send_with_callback(ch, repeat);
        }

        // Wait for all coroutines...
        // !!! user should ensure there is no race for destroying channel !!!
        group.wait();

        // for same read/write operation,
        //  channel guarantees all reader/writer will be executed.
        REQUIRE(failure == 0);

        // however, the mutex in the channel is for matching of the coroutines.
        // so the counter in the context will be raced
        REQUIRE(success <= 2 * max_try_count);
        REQUIRE(success > 0);
    }
};

#if defined(_WINDOWS)
section::section() noexcept(false)
{
    InitializeCriticalSectionAndSpinCount(&cs, 0600);
}
section::~section() noexcept
{
    DeleteCriticalSection(&cs);
}
bool section::try_lock() noexcept
{
    return TryEnterCriticalSection(&cs);
}
void section::lock() noexcept(false)
{
    EnterCriticalSection(&cs);
}
void section::unlock() noexcept(false)
{
    LeaveCriticalSection(&cs);
}
#else
section::section() noexcept(false) : cs{}
{
    if (auto ec = pthread_rwlock_init(&cs, nullptr))
        throw std::system_error{ec, std::system_category(),
                                "pthread_rwlock_init"};
}

section ::~section() noexcept try
{
    if (auto ec = pthread_rwlock_destroy(&cs))
        throw std::system_error{ec, std::system_category(),
                                "pthread_rwlock_init"};
}
catch (const std::system_error& e)
{
    ::perror(e.what());
}
catch (...)
{
    ::perror("Unknown exception in section dtor");
}

bool section ::try_lock() noexcept
{
    // EBUSY  // possible error
    // EINVAL
    // EDEADLK
    auto ec = pthread_rwlock_trywrlock(&cs);
    return ec == 0;
}

// - Note
//
//  There was an issue with `pthread_mutex_`
//  it returned EINVAL for lock operation
//  replacing it the rwlock
//
void section ::lock() noexcept(false)
{
    if (auto ec = pthread_rwlock_wrlock(&cs))
        // EINVAL ?
        throw std::system_error{ec, std::system_category(),
                                "pthread_rwlock_wrlock"};
}

void section ::unlock() noexcept(false)
{
    if (auto ec = pthread_rwlock_unlock(&cs))
        throw std::system_error{ec, std::system_category(),
                                "pthread_rwlock_unlock"};
}
#endif
