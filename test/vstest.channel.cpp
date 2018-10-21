// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------

#include <coroutine/channel.hpp>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

#include <CppUnitTest.h>
#include <sdkddkver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(ChannelTest)
{
    // - Note
    //      Lockable without lock operation
    struct bypass_lock
    {
        bool try_lock() noexcept { return true; }
        void lock() noexcept {}
        void unlock() noexcept {}
    };

    template<typename Ty, typename M>
    auto WriteAndCheck(channel<Ty, M> & ch, Ty value) noexcept->unplug
    {
        bool ok = co_await ch.write(value);
        Assert::IsTrue(ok);
    };

    template<typename Ty, typename M>
    auto ReadAndCheck(channel<Ty, M> & ch, Ty & ref) noexcept->unplug
    {
        bool ok = false;
        std::tie(ref, ok) = co_await ch.read();
        Assert::IsTrue(ok);
    };

  public:
    TEST_METHOD(ReadAfterWrite)
    {
        channel<int, bypass_lock> ch{};
        int value = 0;

        // Writer coroutine may suspend.(not-returned)
        for (int i = 0; i < 4; ++i)
        {
            WriteAndCheck(ch, i);
        }
        // Reader coroutine takes value from Writer coroutine
        for (int i = 0; i < 4; ++i)
        {
            ReadAndCheck(ch, value);
            Assert::IsTrue(value == i);
        }
    }
    TEST_METHOD(WriteAfterRead)
    {
        channel<int, std::mutex> ch{};
        int value = 0;

        // Reader coroutine may suspend.(not-returned)
        for (int i = 0; i < 4; ++i)
        {
            ReadAndCheck(ch, value);
        }
        // Writer coroutine takes value from Reader coroutine
        for (int i = 0; i < 4; ++i)
        {
            WriteAndCheck(ch, i);
            Assert::IsTrue(value == i);
        }
    }

    // use Windows critical section for this test.
   // std::mutex is also available
    using channel_type = channel<char, section>;

public:
    auto Send(channel_type & channel,
        channel_type::value_type value,
        std::function<void(bool)> callback) noexcept->unplug
    {
        switch_to background{};
        co_await background; // go to background

        // Returns false if channel is destroyed
        const bool sent = co_await channel.write(value);
        callback(sent);
    };

    auto Recv(channel_type & channel,
        std::function<void(bool)> callback) noexcept->unplug
    {
        switch_to background{};
        co_await background; // go to background

        channel_type::value_type storage{};
        bool received = false;

        // Returns false if channel is destroyed
        std::tie(storage, received) = co_await channel.read();
        callback(received);
    };

    TEST_METHOD(CountAccess)
    {
        static constexpr size_t Amount = 100'000;

        // Channel supports MT-safe coroutine relay with **appropriate
        // lockable**. but it doesn't guarantee coroutines' serialized execution
        channel_type channel{};

        // So we have to use atomic counter
        std::atomic_uint32_t success = 0, failure = 0;

        wait_group group{};
        group.add(2 * Amount);

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
        for (auto i = 0u; i < Amount; ++i)
        {
            Send(channel, channel_type::value_type{}, callback);
            Recv(channel, callback);
        }
        // Wait for all coroutines...
        // !!! Notice that channel there must be no race before destruction of
        // it
        // !!!
        group.wait();

        Assert::IsTrue(failure == 0);
        Assert::IsTrue(success == 2 * Amount);
    }
};

