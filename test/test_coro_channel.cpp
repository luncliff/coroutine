//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> no_return {
    ok = co_await ch.write(value);
    expect_true(ok);
}

template <typename E, typename L>
auto read_from(channel<E, L>& ch, E& value, bool ok = false) -> no_return {

    tie(value, ok) = co_await ch.read();
    expect_true(ok);
}

template <typename T, typename M, typename Ctr>
auto write_and_count(channel<T, M>& ch, T value, //
                     Ctr& success, Ctr& fail_with_messageure) -> no_return {
    bool ok = co_await ch.write(value);
    if (ok)
        success += 1;
    else
        fail_with_messageure += 1;
}

template <typename T, typename M, typename Ctr>
auto read_and_count(channel<T, M>& ch, T& ref, //
                    Ctr& success, Ctr& fail_with_messageure) -> no_return {
    auto [value, ok] = co_await ch.read();
    if (ok == false) {
        fail_with_messageure += 1;
        co_return;
    }
    ref = value;
    success += 1;
}

class coro_channel_write_before_read_test : public test_adapter {
    using value_type = int;
    using channel_without_lock_t = channel<value_type>;

  public:
    void on_test() override {
        const auto list = {1, 2, 3};
        channel_without_lock_t ch{};
        value_type storage = 0;

        for (auto i : list) {
            write_to(ch, i);           // Writer coroutine will suspend
            expect_true(storage != i); // so no write occurs
        }
        for (auto i : list) {
            read_from(ch, storage);    // read to `storage`
            expect_true(storage == i); // stored value is same with sent value
        }
    }
};
class coro_channel_read_before_write_test : public test_adapter {
    using value_type = int;
    using channel_without_lock_t = channel<value_type>;

  public:
    void on_test() override {
        const auto list = {1, 2, 3};
        channel_without_lock_t ch{};
        value_type storage = 0;

        for (auto i : list) {
            read_from(ch, storage);    // Reader coroutine will suspend
            expect_true(storage != i); // so no read occurs
        }
        for (auto i : list) {
            write_to(ch, i);           // writer will send a value
            expect_true(storage == i); // stored value is same with sent value
        }
    }
};

class coro_channel_mutexed_write_before_read_test : public test_adapter {
    using value_type = int;
    using channel_with_lock_t = channel<value_type, mutex>;

  public:
    void on_test() override {
        const auto list = {1, 2, 3};
        channel_with_lock_t ch{};
        value_type storage = 0;

        for (auto i : list) {
            write_to(ch, i);           // Writer coroutine will suspend
            expect_true(storage != i); // so no write occurs
        }
        for (auto i : list) {
            read_from(ch, storage);    // read to `storage`
            expect_true(storage == i); // stored value is same with sent value
        }
    }
};
class coro_channel_mutexed_read_before_write_test : public test_adapter {
    using value_type = int;
    using channel_with_lock_t = channel<value_type, mutex>;

  public:
    void on_test() override {
        const auto list = {1, 2, 3};
        channel_with_lock_t ch{};
        value_type storage = 0;

        for (auto i : list) {
            read_from(ch, storage);    // Reader coroutine will suspend
            expect_true(storage != i); // so no read occurs
        }
        for (auto i : list) {
            write_to(ch, i);           // writer will send a value
            expect_true(storage == i); // stored value is same with sent value
        }
    }
};

class coro_channel_write_return_false_after_close_test : public test_adapter {
    using value_type = int;
    using channel_without_lock_t = channel<value_type>;

  public:
    void on_test() override {
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
        expect_true(ch.get() == nullptr);

        coroutine_handle<void> coro = h;
        expect_true(coro.done()); // coroutine is in done state
        coro.destroy();           // destroy to prevent leak

        expect_true(ok == false); // and channel returned false
    }
};

class coro_channel_read_return_false_after_close_test : public test_adapter {
    using value_type = int;
    using channel_without_lock_t = channel<value_type>;

  public:
    void on_test() override {
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
        expect_true(ch.get() == nullptr);

        coroutine_handle<void> coro = h;
        expect_true(coro.done()); // coroutine is in done state
        coro.destroy();           // destroy to prevent leak

        expect_true(ok == false); // and channel returned false
    }
};

class coro_channel_select_type_matching_test : public test_adapter {
    using u32_chan_t = channel<uint32_t>;
    using i32_chan_t = channel<int32_t>;

  public:
    void on_test() override {
        u32_chan_t ch1{};
        i32_chan_t ch2{};
        write_to(ch1, 17u);

        // the callback functions can be a coroutine (which includes
        // subroutine), but their returns will be truncated !
        select(ch2,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), int32_t>);
                   fail_with_message("select on empty channel must bypass");
               },
               ch1,
               [](auto v) -> no_return {
                   static_assert(is_same_v<decltype(v), uint32_t>);
                   expect_true(v == 17u);

                   co_await suspend_never{};
               });
    }
};

class coro_channel_select_on_empty_test : public test_adapter {
    using u32_chan_t = channel<uint32_t>;
    using i32_chan_t = channel<int32_t>;

  public:
    void on_test() override {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        select(ch1,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), uint32_t>);
                   fail_with_message("select on empty channel must bypass");
               },
               ch2,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), int32_t>);
                   fail_with_message("select on empty channel must bypass");
               });
    }
};

class coro_channel_select_peek_every_test : public test_adapter {
    using u32_chan_t = channel<uint32_t>;
    using i32_chan_t = channel<int32_t>;

  public:
    void on_test() override {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        write_to(ch1, 17u);
        write_to(ch2, 15);

        select(ch2, [](auto v) { expect_true(v == 15); }, //
               ch1, [](auto v) { expect_true(v == 17u); } //
        );
    }
};

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
class ptp_work final { // work like `ptp_work` of win32 based interface
    coroutine_handle<void> coro{};

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
#endif // follow windows on unix

class coro_channel_no_leak_under_race_test : public test_adapter {
    using value_type = uint64_t;
    using channel_type = channel<value_type, section>;

    static constexpr size_t max_try_count = 6;

  public:
    void on_test() override {
        uint32_t success{}, fail_with_messageure{};
        latch group{2 * max_try_count};

        auto send_with_callback = [&](channel_type& ch,
                                      value_type value) -> no_return {
            co_await ptp_work{};

            auto w = co_await ch.write(value);
            w ? success += 1 : fail_with_messageure += 1;
            group.count_down();
        };
        auto recv_with_callback = [&](channel_type& ch) -> no_return {
            co_await ptp_work{};

            auto [value, r] = co_await ch.read();
            r ? success += 1 : fail_with_messageure += 1;
            group.count_down();
        };

        channel_type ch{};

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
        expect_true(fail_with_messageure == 0);

        // however, the mutex in the channel is for matching of the coroutines.
        // so the counter in the context will be raced
        expect_true(success <= 2 * max_try_count);
        expect_true(success > 0);
    }
};
