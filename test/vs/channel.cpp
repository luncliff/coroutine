// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/channel.hpp>
#include <coroutine/return.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

#include <Windows.h>
#include <sdkddkver.h>

#include <CppUnitTest.h>
#include <TlHelp32.h>
#include <threadpoolapiset.h>

using namespace std::literals;
using namespace std::experimental;

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

class channel_operation_test : public TestClass<channel_operation_test>
{
    // - Note
    //      Lockable without lock operation
    class bypass_lock final
    {
      public:
        bool try_lock() noexcept
        {
            return true;
        }
        void lock() noexcept
        {
            // this lock does nothing
        }
        void unlock() noexcept
        {
            // we didn't locked, so nothing to do
        }
    };

    using channel_type = channel<uint64_t, bypass_lock>;

    static auto write_to(channel_type& ch, uint64_t value, bool ok = false)
        -> unplug
    {
        ok = co_await ch.write(value);
        Assert::IsTrue(ok);
    }

    static auto read_from(channel_type& ch, uint64_t& value, bool ok = false)
        -> unplug
    {
        std::tie(value, ok) = co_await ch.read();
        Assert::IsTrue(ok);
    }

    static auto write_to(channel_type& ch, uint64_t value, uint32_t& success,
                         uint32_t& failure) -> unplug
    {
        if (co_await ch.write(value))
            success += 1;
        else
            failure += 1;
    }

    static auto read_from(channel_type& ch, uint64_t& ref, uint32_t& success,
                          uint32_t& failure) -> unplug
    {
        auto [value, ok] = co_await ch.read();
        if (ok == false)
        {
            failure += 1;
            co_return;
        }

        ref = value;
        success += 1;
    }

  public:
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

            for (auto i = 0U; i < 2; ++i)
                read_from(ch, storage, success, failure);

            // write more than read
            for (auto i = 0U; i < 3; ++i)
                write_to(ch, i, success, failure);
        }
        // channel returns false to writer when it's going to destroy
        Assert::IsTrue(success == 4);
        Assert::IsTrue(failure == 1);
    }

    TEST_METHOD(channel_cancel_read_when_destroy)
    {
        uint64_t storage{};
        uint32_t success{};
        uint32_t failure{};
        {
            channel_type ch{};

            // read more than write
            for (auto i = 0U; i < 3; ++i)
                read_from(ch, storage, success, failure);

            for (auto i = 0U; i < 2; ++i)
                write_to(ch, i, success, failure);
        }
        // channel returns false to reader when it's going to destroy
        Assert::IsTrue(success == 4);
        Assert::IsTrue(failure == 1);
    }
};

class channel_race_test : public TestClass<channel_race_test>
{
    using channel_type = channel<uint64_t, section>;

  public:
    TEST_METHOD(channel_ensure_delivery_under_race)
    {
        constexpr size_t max_try_count = 2'000;

        // using 'guaranteed' lockable
        // but it doesn't guarantee coroutines' serialized execution
        channel_type ch{};
        uint32_t success{};
        uint32_t failure{};

        wait_group group{};
        switch_to back{};
        scheduler_t& scheduler = back.scheduler();

        group.add(2 * max_try_count);

        auto send_with_callback
            = [&back](auto& ch, auto value, auto fn) -> unplug {
            co_await back; // move to background

            const auto ok = co_await ch.write(value);
            fn(ok);
        };
        auto recv_with_callback = [&back](auto& ch, auto fn) -> unplug {
            co_await back; // move to background

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

            group.done(); // notify the end of coroutine
        };

        // Spawn coroutines
        uint64_t repeat = max_try_count;
        while (repeat--)
        {
            recv_with_callback(ch, callback);
            send_with_callback(ch, repeat, callback);
        }

        // Wait for all coroutines...
        // !!! user should ensure there is no race for destroying channel !!!
        Assert::IsTrue(group.wait(10s));

        scheduler.close();

        // channel ensures the delivery for same number of send/recv
        Assert::IsTrue(failure == 0);
        // but the race makes the caks success != 2 * max_try_count
        Assert::IsTrue(success > 0);
        Assert::IsTrue(success <= 2 * max_try_count);
    }
};
