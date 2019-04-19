// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/channel.hpp>
#include <coroutine/concrt.h>
#include <coroutine/return.h>

// clang-format off
#include <Windows.h>
#include <sdkddkver.h>
#include <CppUnitTest.h>
#include <threadpoolapiset.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
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
    using channel_type = channel<uint64_t, bypass_lock>;

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
    using u32_chan_t = channel<uint32_t, bypass_lock>;
    using i32_chan_t = channel<int32_t, bypass_lock>;

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

// - Note
//      Basic lockable for criticial section
class section final : public ::CRITICAL_SECTION
{
  private:
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

  public:
    section() noexcept(false)
    {
        InitializeCriticalSectionAndSpinCount(this, 0600);
    }
    ~section() noexcept
    {
        DeleteCriticalSection(this);
    }

    bool try_lock() noexcept
    {
        return TryEnterCriticalSection(this);
    }
    void lock() noexcept(false)
    {
        EnterCriticalSection(this);
    }
    void unlock() noexcept(false)
    {
        LeaveCriticalSection(this);
    }
};

class channel_race_test : public TestClass<channel_race_test>
{
    using channel_type = channel<uint64_t, section>;
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

    static auto create_write_work(context& test_context)
    {
        return ::CreateThreadpoolWork(
            [](PTP_CALLBACK_INSTANCE, void* ptr, PTP_WORK work) {
                auto* ctx = reinterpret_cast<context*>(ptr);
                ctx->write();

                ::CloseThreadpoolWork(work);
            },
            addressof(test_context), nullptr);
    }

    static auto create_read_work(context& test_context)
    {
        return ::CreateThreadpoolWork(
            [](PTP_CALLBACK_INSTANCE, void* ptr, PTP_WORK work) {
                auto* ctx = reinterpret_cast<context*>(ptr);
                ctx->read();

                ::CloseThreadpoolWork(work);
            },
            addressof(test_context), nullptr);
    }

    TEST_METHOD(channel_under_race)
    {
        // issue: when work amount is over 30,
        //	access vioation is detected at `write_and_count`
        //  finding the reason...
        uint32_t num_of_work = 30;

        context ctx{2 * num_of_work};
        // simple fork-join
        for (auto i = 0u; i < num_of_work; ++i)
        {
            ::SubmitThreadpoolWork(create_read_work(ctx));
            ::SubmitThreadpoolWork(create_write_work(ctx));
        }
        ctx.wg.wait();

        // for same read/write operation,
        //  channel guarantees all reader/writer will be executed.
        Assert::IsTrue(ctx.failure == 0);

        // however, the mutex in the channel is for matching of the coroutines.
        // so the counter in the context will be raced
        Assert::IsTrue(ctx.success > 0);
        Assert::IsTrue(ctx.success <= 2 * num_of_work);
    }
};
