// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      This file is distributed under Creative Commons 4.0-BY License
//
// ---------------------------------------------------------------------------
#include <magic/sync.h>
#include <magic/switch.h>
#include <magic/channel.hpp>

#include <sdkddkver.h>
#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace magic
{
using namespace std;

TEST_CLASS(ChannelOperationTest)
{
    template <typename Ty, typename M>
    auto WriteAndCheck(channel<Ty, M> & ch, Ty value) noexcept
        ->unplug
    {
        bool ok = co_await ch.write(value);
        Assert::IsTrue(ok, L"Write to channel");
    };

    template <typename Ty, typename M>
    auto ReadAndCheck(channel<Ty, M> & ch, Ty & ref) noexcept
        ->unplug
    {
        bool ok = false;
        std::tie(ref, ok) = co_await ch.read();
        Assert::IsTrue(ok, L"Read from channel");
    };

  public:
    TEST_METHOD(ChannelReadAfterWrite)
    {
        channel<int, std::mutex> ch{};
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
            Assert::IsTrue(value == i, L"Expected read result");
        }
    }
    TEST_METHOD(ChannelWriteAfterRead)
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
            Assert::IsTrue(value == i, L"Expected write result");
        }
    }
};

TEST_CLASS(ChannelRaceTest)
{
    static constexpr size_t NumWorker = 1'000;
    static constexpr size_t Repeat = 100;

    auto Write1(channel<char, std::mutex> & chan, wait_group & grp,
                atomic<size_t> & counter)
        ->unplug
    {
        char value{};

        co_await switch_to{};

        for (size_t i = 0; i < Repeat; ++i)
        {
            // Returns false if channel is destroyed
            const bool open = co_await chan.write(value);
            if (open == false)
                break;

            counter += 1;
        }
        grp.done();
    };

    auto Read1(channel<char, std::mutex> & chan, wait_group & grp,
               size_t & counter)
        ->unplug
    {
        int value{};
        bool open = false;

        co_await switch_to{};

        for (size_t i = 0; i < Repeat; ++i)
        {
            // Returns false if channel is destroyed
            std::tie(value, open) = co_await chan.read();
            if (open == false)
                break;

            counter += 1;
        }
        grp.done();
    };

    auto Write2(channel<char, std::mutex> & chan, wait_group & group,
                atomic<size_t> & success, atomic<size_t> & failure)
        ->unplug
    {
        char value{};

        co_await switch_to{};

        for (size_t i = 0; i < Repeat; ++i)
        {
            // Returns false if channel is destroyed
            const bool open = co_await chan.write(value);
            if (open == false)
            {
                failure += 1;
                group.done();
                return;
            }
        }
        success += 1;
        group.done();
        return;
    };

    auto Read2(channel<char, std::mutex> & chan, wait_group & group,
               atomic<size_t> & success)
        ->unplug
    {
        char value{};
        bool open = false;

        co_await switch_to{};

        for (size_t i = 0; i < Repeat; ++i)
        {
            // Returns false if channel is destroyed
            std::tie(value, open) = co_await chan.read();
            if (open == false)
                throw std::runtime_error{"Read failed"};
        }
        success += 1;
        group.done();
    };

    auto Write3(channel<char, std::mutex> & chan, wait_group & group,
                atomic<size_t> & success)
        ->unplug
    {
        char value{};

        co_await switch_to{};

        for (size_t i = 0; i < Repeat; ++i)
        {
            // Returns false if channel is destroyed
            const bool open = co_await chan.write(value);
            if (open == false)
                throw std::runtime_error{"Write failed"};
        }
        success += 1;
        group.done();
    };

    auto Read3(channel<char, std::mutex> & chan, wait_group & group,
               atomic<size_t> & success, atomic<size_t> & failure)
        ->unplug
    {
        int value{};
        bool open = false;

        co_await switch_to{};

        for (size_t i = 0; i < Repeat; ++i)
        {
            // Returns false if channel is destroyed
            std::tie(value, open) = co_await chan.read();
            if (open == false)
            {
                failure += 1;
                group.done();
                return;
            }
        }
        success += 1;
        group.done();
        return;
    };

  public:
    TEST_METHOD(ChannelWriterEqualsReader)
    {
        channel<char, std::mutex> ch{};
        wait_group wg{};

        size_t nc = 0;         // normal counter
        atomic<size_t> ac = 0; // atomic counter

        wg.add(2 * NumWorker);

        // Spawn coroutines
        // counters will be under race condition
        //  - Senders use normal counter
        //  - Receivers use atomic counter
        for (int i = 0; i < NumWorker; ++i)
        {
            Write1(ch, wg, ac);
            Read1(ch, wg, nc);
        }
        // Wait for all coroutines...
        wg.wait();

        // Channel supports MT-safe coroutine relay,
        // but it doesn't guarantee coroutines' serialized execution
        Assert::IsTrue(nc <= ac);
    }

    TEST_METHOD(ChannelWriterGreaterThanReader)
    {
        // Counter for success & failure
        atomic<size_t> successW = 0;
        atomic<size_t> successR = 0;

        atomic<size_t> failure = 0;

        // !!! This code leads to coroutine leak !!!

        wait_group reader_group{};
        wait_group writer_group{};
        {
            channel<char, std::mutex> ch{};

            reader_group.add(1 * NumWorker);
            writer_group.add(2 * NumWorker);

            for (int i = 0; i < NumWorker; ++i)
            {
                Write2(ch, writer_group, successW, failure);
                Read2(ch, reader_group, successR);
                Write2(ch, writer_group, successW, failure); // Spawn more writer
            }
            // Wait for all coroutines...
            reader_group.wait();

        } // channel is dead
        writer_group.wait();

        Assert::IsTrue(successW + successR + failure <= 3 * NumWorker);
    }

    TEST_METHOD(ChannelWriterLessThanReader)
    {

        // Counter for success & failure
        atomic<size_t> successW = 0;
        atomic<size_t> successR = 0;
        atomic<size_t> failure = 0;

        wait_group reader_group{};
        wait_group writer_group{};
        {
            channel<char, std::mutex> ch{};

            reader_group.add(2 * NumWorker);
            writer_group.add(1 * NumWorker);

            for (int i = 0; i < NumWorker; ++i)
            {
                Read3(ch, reader_group, successR, failure);
                Write3(ch, writer_group, successW);
                Read3(ch, reader_group, successR, failure); // Spawn more reader
            }
            // Wait for all coroutines...
            writer_group.wait();

        } // channel is dead
        reader_group.wait();

        Assert::IsTrue(successW + successR + failure <= 3 * NumWorker);
    }
};
} // namespace magic
