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

    template <typename L>
    auto write_to(channel<uint64_t, L>& ch, uint64_t value, bool ok = false)
        -> unplug
    {
        ok = co_await ch.write(value);
        Assert::IsTrue(ok);
    }

    template <typename L>
    auto read_from(channel<uint64_t, L>& ch, uint64_t& value, bool ok = false)
        -> unplug
    {
        std::tie(value, ok) = co_await ch.read();
        Assert::IsTrue(ok);
    }

  public:
    TEST_METHOD(channel_write_before_read)
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
            // stored value is same with sent value
            Assert::IsTrue(storage == i);
        }
    }

    TEST_METHOD(channel_read_before_write)
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
            // stored value is same with sent value
            Assert::IsTrue(storage == i);
        }
    }
};

class channel_race_test : public TestClass<channel_race_test>
{
  public:
    TEST_METHOD(channel_ensure_delivery)
    {
        // using 'guaranteed' lockable
        // but it doesn't guarantee coroutines' serialized execution
        channel<uint64_t, section> ch{};
        uint32_t success = 0, failure = 0;

        static constexpr size_t TryCount = 2'000;

        wait_group group{};
        group.add(2 * TryCount);

        auto send_with_callback = [](auto& ch, auto value, auto fn) -> unplug {
            switch_to back{};
            co_await back;

            const auto ok = co_await ch.write(value);
            fn(ok);
        };
        auto recv_with_callback = [](auto& ch, auto fn) -> unplug {
            switch_to back{};
            co_await back;

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

        const auto timeout = std::chrono::seconds{10};
        // Wait for all coroutines...
        // !!! use should ensure there is no race for destroying channel !!!
        Assert::IsTrue(group.wait(timeout));

        // channel ensures the delivery for same number of send/recv
        Assert::IsTrue(failure == 0);
        // but the race makes the caks success != 2 * TryCount
        Assert::IsTrue(success <= 2 * TryCount);
    }
};
