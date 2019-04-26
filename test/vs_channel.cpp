// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/channel.hpp>
#include <coroutine/concrt.h>

#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace std::literals;

using namespace std::experimental;
using namespace coro;
using namespace concrt;

template <typename T, typename M>
auto write_to(channel<T, M>& ch, T value, bool ok = false) -> return_ignore
{
    ok = co_await ch.write(value);
    Assert::IsTrue(ok);
}

template <typename T, typename M>
auto read_from(channel<T, M>& ch, T& value, bool ok = false) -> return_ignore
{
    tie(value, ok) = co_await ch.read();
    Assert::IsTrue(ok);
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

class channel_operation_test : public TestClass<channel_operation_test>
{
    using channel_type = channel<uint64_t>;

    TEST_METHOD(channel_write_before_read)
    {
        uint64_t storage = 0;
        channel_type ch{};

        // Writer coroutine may suspend.(not-returned)
        for (uint64_t i = 0U; i < 3; ++i)
            write_to(ch, i);

        for (uint64_t i = 0U; i < 3; ++i)
        {
            // read to `storage`
            read_from(ch, storage);
            // stored value is same with sent value
            Assert::IsTrue(storage == i);
        }
    }

    TEST_METHOD(channel_read_before_write)
    {
        uint64_t storage = 0;
        channel_type ch{};

        // Reader coroutine may suspend.(not-returned)
        for (uint64_t i = 0U; i < 3; ++i)
            // read to `storage`
            read_from(ch, storage);

        for (uint64_t i = 0U; i < 3; ++i)
        {
            write_to(ch, i);
            // stored value is same with sent value
            Assert::IsTrue(storage == i);
        }
    }

    TEST_METHOD(channel_cancel_write_when_destroy)
    {
        uint64_t storage{};
        uint32_t success{};
        uint32_t failure{};
        {
            channel_type ch{};

            for (auto i = uint64_t{}; i < 2; ++i)
                read_and_count(ch, storage, success, failure);

            // write more than read
            for (auto i = uint64_t{}; i < 3; ++i)
                write_and_count(ch, i, success, failure);
        }
        // channel returns false to writer when it's going to destroy
        Assert::IsTrue(success == 4); // 2 read + 2 write
        Assert::IsTrue(failure == 1); // 1 write
    }

    TEST_METHOD(channel_cancel_read_when_destroy)
    {
        uint64_t storage{};
        uint32_t success{};
        uint32_t failure{};
        {
            channel_type ch{};

            // read more than write
            for (auto i = uint64_t{}; i < 3; ++i)
                read_and_count(ch, storage, success, failure);

            for (auto i = uint64_t{}; i < 2; ++i)
                write_and_count(ch, i, success, failure);
        }
        // channel returns false to reader when it's going to destroy
        Assert::IsTrue(success == 4); // 2 read + 2 write
        Assert::IsTrue(failure == 1); // 1 write
    }
};

class channel_select_test : public TestClass<channel_select_test>
{
    // it's singe thread, so mutex for channels doesn't have to be real lockable
    using u32_chan_t = channel<uint32_t>;
    using i32_chan_t = channel<int32_t>;

    TEST_METHOD(channel_select_match_one)
    {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        write_to(ch1, 17u);
        select(ch2, // empty channel. bypass
               [](auto v) {
                   static_assert(is_same_v<decltype(v), int32_t>);
                   Assert::Fail(L"select on empty channel must bypass");
               },
               ch1, // if the channel has a writer, peek and invoke callback
               [](auto v) -> return_ignore {
                   static_assert(is_same_v<decltype(v), uint32_t>);
                   Assert::IsTrue(v == 17u);

                   co_await suspend_never{};
               });
    }

    TEST_METHOD(channel_select_no_match)
    {
        u32_chan_t ch1{};
        i32_chan_t ch2{};

        // both channels are empty. no callback execution
        select(ch1,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), uint32_t>);
                   Assert::Fail(L"select on empty channel must bypass");
               },
               ch2,
               [](auto v) {
                   static_assert(is_same_v<decltype(v), int32_t>);
                   Assert::Fail(L"select on empty channel must bypass");
               });
    }

    TEST_METHOD(channel_select_match_both)
    {
        u32_chan_t ch1{};
        i32_chan_t ch2{};
        bool ch1_ok = false;
        bool ch2_ok = false;

        write_to(ch1, 17u);
        write_to(ch2, 15);

        // both callback will be executed
        select(ch2, [&](auto v) { ch2_ok = (v == 15); }, //
               ch1, [&](auto v) { ch1_ok = (v == 17u); } //
        );
        Assert ::IsTrue(ch1_ok);
        Assert ::IsTrue(ch2_ok);
    }
};

class channel_race_test : public TestClass<channel_race_test>
{
    using value_type = uint64_t;
    using channel_type = channel<value_type, section>;
    using wait_group = concrt::latch;

  public:
    TEST_METHOD(channel_no_leak_under_race)
    {
        // see <coroutine/concrt.h>
        using namespace concrt;

        // using 'guaranteed' lockable
        // however, it doesn't guarantee coroutines' serialized execution
        channel_type ch{};
        uint32_t success{};
        uint32_t failure{};

        static constexpr size_t max_try_count = 6'000;
        wait_group group{2 * max_try_count};

        auto send_with_callback
            = [&](channel_type& ch, value_type value) -> return_ignore {
            co_await ptp_work{};

            auto w = co_await ch.write(value);
            w ? success += 1 : failure += 1;
            group.count_down();
        };
        auto recv_with_callback = [&](channel_type& ch) -> return_ignore {
            co_await ptp_work{};

            auto [value, r] = co_await ch.read();
            r ? success += 1 : failure += 1;
            group.count_down();
        };

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

        // channel ensures the delivery for same number of send/recv
        Assert::IsTrue(failure == 0);
        // but the race makes the caks success != 2 * max_try_count
        Assert::IsTrue(success <= 2 * max_try_count);
        Assert::IsTrue(success > 0);
    }
};
