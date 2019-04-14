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
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

#include "./channel_test.h"

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void test_require_true(bool cond)
{
    Assert::IsTrue(cond);
}

class channel_operation_test : public TestClass<channel_operation_test>
{
    using channel_type = channel<uint64_t, bypass_lock>;

    static auto write_to(channel_type& ch, uint64_t value, bool ok = false)
        -> return_ignore
    {
        ok = co_await ch.write(value);
        test_require_true(ok);
    }

    static auto read_from(channel_type& ch, uint64_t& value, bool ok = false)
        -> return_ignore
    {
        std::tie(value, ok) = co_await ch.read();
        test_require_true(ok);
    }

    static auto write_to(channel_type& ch, uint64_t value, uint32_t& success,
                         uint32_t& failure) -> return_ignore
    {
        if (co_await ch.write(value))
            success += 1;
        else
            failure += 1;
    }

    static auto read_from(channel_type& ch, uint64_t& ref, uint32_t& success,
                          uint32_t& failure) -> return_ignore
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
