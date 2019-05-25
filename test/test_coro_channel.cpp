//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

using value_type = int;
using channel_without_lock_t = channel<value_type>;
using channel_with_lock_t = channel<value_type, mutex>;
using u32_chan_t = channel<uint32_t>;
using i32_chan_t = channel<int32_t>;
using channel_section_t = channel<uint64_t, section>;

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
// work like `ptp_work` of win32 based interface, but rely on standard
class ptp_work final : no_copy_move {
    coroutine_handle<void> coro;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(coroutine_handle<void> handle) noexcept(false) {
        coro = handle;
        async(launch::async, [this]() { coro.resume(); });
    }
    void await_resume() noexcept {
        coro = nullptr; // forget it
    }
};
#endif

template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> no_return {
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

template <typename E, typename L>
auto read_from(channel<E, L>& ch, E& value, bool ok = false) -> no_return {

    tie(value, ok) = co_await ch.read();
    REQUIRE(ok);
}

template <typename T, typename M, typename Ctr>
auto write_and_count(channel<T, M>& ch, T value, //
                     Ctr& success, Ctr& failure) -> no_return {
    bool ok = co_await ch.write(value);
    if (ok)
        success += 1;
    else
        failure += 1;
}

template <typename T, typename M, typename Ctr>
auto read_and_count(channel<T, M>& ch, T& ref, //
                    Ctr& success, Ctr& failure) -> no_return {
    auto [value, ok] = co_await ch.read();
    if (ok == false) {
        failure += 1;
        co_return;
    }
    ref = value;
    success += 1;
}

auto coro_channel_write_before_read_test() {

    const auto list = {1, 2, 3};
    channel_without_lock_t ch{};
    value_type storage = 0;

    for (auto i : list) {
        write_to(ch, i);       // Writer coroutine will suspend
        REQUIRE(storage != i); // so no write occurs
    }
    for (auto i : list) {
        read_from(ch, storage); // read to `storage`
        REQUIRE(storage == i);  // stored value is same with sent value
    }
}

auto coro_channel_read_before_write_test() {
    const auto list = {1, 2, 3};
    channel_without_lock_t ch{};
    value_type storage = 0;

    for (auto i : list) {
        read_from(ch, storage); // Reader coroutine will suspend
        REQUIRE(storage != i);  // so no read occurs
    }
    for (auto i : list) {
        write_to(ch, i);       // writer will send a value
        REQUIRE(storage == i); // stored value is same with sent value
    }
}

auto coro_channel_mutexed_write_before_read_test() {
    const auto list = {1, 2, 3};
    channel_with_lock_t ch{};
    value_type storage = 0;

    for (auto i : list) {
        write_to(ch, i);       // Writer coroutine will suspend
        REQUIRE(storage != i); // so no write occurs
    }
    for (auto i : list) {
        read_from(ch, storage); // read to `storage`
        REQUIRE(storage == i);  // stored value is same with sent value
    }
}

auto coro_channel_mutexed_read_before_write_test() {
    const auto list = {1, 2, 3};
    channel_with_lock_t ch{};
    value_type storage = 0;

    for (auto i : list) {
        read_from(ch, storage); // Reader coroutine will suspend
        REQUIRE(storage != i);  // so no read occurs
    }
    for (auto i : list) {
        write_to(ch, i);       // writer will send a value
        REQUIRE(storage == i); // stored value is same with sent value
    }
}

auto coro_channel_write_return_false_after_close_test() {
    auto ch = make_unique<channel_without_lock_t>();
    bool ok = true;

    auto coro_write = [&ok](auto& ch, auto value) -> frame {
        ok = co_await ch.write(value);
    };
    // coroutine will suspend and wait in the channel
    auto h = coro_write(*ch, value_type{});
    {
        auto truncator = move(ch); // if channel is destroyed ...
    }
    REQUIRE(ch.get() == nullptr);

    coroutine_handle<void> coro = h;
    REQUIRE(coro.done()); // coroutine is in done state
    coro.destroy();       // destroy to prevent leak

    REQUIRE(ok == false); // and channel returned false
}

auto coro_channel_read_return_false_after_close_test() {
    auto ch = make_unique<channel_without_lock_t>();
    bool ok = true;

    auto coro_read = [&ok](auto& ch, auto& value) -> frame {
        tie(value, ok) = co_await ch.read();
    };

    auto item = value_type{};
    // coroutine will suspend and wait in the channel
    auto h = coro_read(*ch, item);
    {
        auto truncator = move(ch); // if channel is destroyed ...
    }
    REQUIRE(ch.get() == nullptr);

    coroutine_handle<void> coro = h;
    REQUIRE(coro.done()); // coroutine is in done state
    coro.destroy();       // destroy to prevent leak

    REQUIRE(ok == false); // and channel returned false
}

auto coro_channel_select_type_matching_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};
    write_to(ch1, 17u);

    // the callback functions can be a coroutine (which includes
    // subroutine), but their returns will be truncated !
    select(ch2,
           [](auto v) {
               static_assert(is_same_v<decltype(v), int32_t>);
               FAIL_WITH_MESSAGE("select on empty channel must bypass"s);
           },
           ch1,
           [](auto v) -> no_return {
               static_assert(is_same_v<decltype(v), uint32_t>);
               REQUIRE(v == 17u);

               co_await suspend_never{};
           });
}

auto coro_channel_select_on_empty_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    select(ch1,
           [](auto v) {
               static_assert(is_same_v<decltype(v), uint32_t>);
               FAIL_WITH_MESSAGE("select on empty channel must bypass"s);
           },
           ch2,
           [](auto v) {
               static_assert(is_same_v<decltype(v), int32_t>);
               FAIL_WITH_MESSAGE("select on empty channel must bypass"s);
           });
}

auto coro_channel_select_peek_every_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    write_to(ch1, 17u);
    write_to(ch2, 15);

    select(ch2, [](auto v) { REQUIRE(v == 15); }, //
           ch1, [](auto v) { REQUIRE(v == 17u); } //
    );
}

auto coro_channel_no_leak_under_race_test() {

    static constexpr size_t max_try_count = 6;

    uint32_t success{}, failure{};
    latch group{2 * max_try_count};

    auto send_with_callback = [&](channel_section_t& ch,
                                  uint64_t value) -> no_return {
        co_await ptp_work{};

        auto w = co_await ch.write(value);
        w ? success += 1 : failure += 1;
        group.count_down();
    };
    auto recv_with_callback = [&](channel_section_t& ch) -> no_return {
        co_await ptp_work{};

        auto [value, r] = co_await ch.read();
        r ? success += 1 : failure += 1;
        group.count_down();
    };

    channel_section_t ch{};

    // Spawn coroutines
    uint64_t repeat = max_try_count;
    while (repeat--) {
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

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("channel write before read", "[channel]") {
    coro_channel_write_before_read_test();
}
TEST_CASE("channel read before write", "[channel]") {
    coro_channel_read_before_write_test();
}
TEST_CASE("channel(with mutex) write before read", "[channel][concurrency]") {
    coro_channel_mutexed_write_before_read_test();
}
TEST_CASE("channel(with mutex) read before write", "[channel][concurrency]") {
    coro_channel_mutexed_read_before_write_test();
}
TEST_CASE("channel close while write", "[channel]") {
    coro_channel_write_return_false_after_close_test();
}
TEST_CASE("channel close while read", "[channel]") {
    coro_channel_read_return_false_after_close_test();
}
TEST_CASE("channel select type assertion", "[channel]") {
    coro_channel_select_type_matching_test();
}
TEST_CASE("channel select bypass empty", "[channel]") {
    coro_channel_select_on_empty_test();
}
TEST_CASE("channel select peek all non-empty", "[channel]") {
    coro_channel_select_peek_every_test();
}
TEST_CASE("channel no leak under race condition", "[channel][concurrency]") {
    coro_channel_no_leak_under_race_test();
}

#elif __has_include(<CppUnitTest.h>)

class coro_channel_write_before_read
    : public TestClass<coro_channel_write_before_read> {
    TEST_METHOD(test_coro_channel_write_before_read) {
        coro_channel_write_before_read_test();
    }
};
class coro_channel_read_before_write
    : public TestClass<coro_channel_read_before_write> {
    TEST_METHOD(test_coro_channel_read_before_write) {
        coro_channel_read_before_write_test();
    }
};
class coro_channel_mutexed_write_before_read
    : public TestClass<coro_channel_mutexed_write_before_read> {
    TEST_METHOD(test_coro_channel_mutexed_write_before_read) {
        coro_channel_mutexed_write_before_read_test();
    }
};
class coro_channel_mutexed_read_before_write
    : public TestClass<coro_channel_mutexed_read_before_write> {
    TEST_METHOD(test_coro_channel_mutexed_read_before_write) {
        coro_channel_mutexed_read_before_write_test();
    }
};
class coro_channel_write_return_false_after_close
    : public TestClass<coro_channel_write_return_false_after_close> {
    TEST_METHOD(test_coro_channel_write_return_false_after_close) {
        coro_channel_write_return_false_after_close_test();
    }
};
class coro_channel_select_type_matching
    : public TestClass<coro_channel_select_type_matching> {
    TEST_METHOD(test_coro_channel_select_type_matching) {
        coro_channel_select_type_matching_test();
    }
};
class coro_channel_select_on_empty
    : public TestClass<coro_channel_select_on_empty> {
    TEST_METHOD(test_coro_channel_select_on_empty) {
        coro_channel_select_on_empty_test();
    }
};
class coro_channel_select_peek_every
    : public TestClass<coro_channel_select_peek_every> {
    TEST_METHOD(test_coro_channel_select_peek_every) {
        coro_channel_select_peek_every_test();
    }
};
class coro_channel_no_leak_under_race
    : public TestClass<coro_channel_no_leak_under_race> {
    TEST_METHOD(test_coro_channel_no_leak_under_race) {
        coro_channel_no_leak_under_race_test();
    }
};

#endif
