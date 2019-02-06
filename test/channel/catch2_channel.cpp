//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include "./channel_test.h"

void test_require_true(bool cond)
{
    REQUIRE(cond);
}

TEST_CASE("channel without lock", "[generic][channel]")
{
    using namespace std;
    using channel_without_lock_t = channel<uint64_t, bypass_lock>;

    channel_without_lock_t ch{};
    uint64_t storage = 0;
    array<uint64_t, 3> nums = {1, 2, 3};

    SECTION("write before read")
    {
        // Writer coroutine may suspend.(not-returned)
        for (auto i : nums)
            write_to(ch, i);

        for (auto i : nums)
        {
            // read to `storage`
            read_from(ch, storage);
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }
    SECTION("read before write")
    {
        // Writer coroutine may suspend.(not-returned)
        for (auto i : nums)
            write_to(ch, i);

        for (auto i : nums)
        {
            // read to `storage`
            read_from(ch, storage);
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }
}

TEST_CASE("channel with mutex", "[generic][channel]")
{
    using namespace std;
    using channel_with_lock_t = channel<uint64_t, mutex>;

    channel_with_lock_t ch{};
    uint64_t storage = 0;
    array<uint64_t, 3> nums = {1, 2, 3};

    SECTION("write before read")
    {
        // Writer coroutine may suspend.(not-returned)
        for (auto i : nums)
            write_to(ch, i);

        for (auto i : nums)
        {
            // read to `storage`
            read_from(ch, storage);
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }

    SECTION("read before write")
    {
        // Writer coroutine may suspend.(not-returned)
        for (auto i : nums)
            write_to(ch, i);

        for (auto i : nums)
        {
            // read to `storage`
            read_from(ch, storage);
            REQUIRE(storage == i); // stored value is same with sent value
        }
    }
}
