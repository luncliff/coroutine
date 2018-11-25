//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include "../modules/shared/queue.hpp"

TEST_CASE("CircularQueueTest", "[messaging]")
{
    using queue_t = circular_queue_t<uint64_t, 3>;
    auto cq = std::make_unique<queue_t>();

    SECTION("EmptyPush")
    {
        REQUIRE(cq->is_empty());
        REQUIRE_FALSE(cq->is_full());

        cq->push(1);
        REQUIRE_FALSE(cq->is_empty());
    }
    SECTION("EmptyPop")
    {
        uint64_t storage{};
        REQUIRE(cq->is_empty());
        REQUIRE_FALSE(cq->is_full());

        REQUIRE_FALSE(cq->pop(storage));
    }
    SECTION("FullPush")
    {
        REQUIRE(cq->push(2));
        REQUIRE(cq->push(3));
        REQUIRE(cq->push(4));

        REQUIRE(cq->is_full());
        REQUIRE_FALSE(cq->is_empty());
        REQUIRE_FALSE(cq->push(5));
    }
    SECTION("FullPop")
    {
        uint64_t storage{};

        REQUIRE(cq->push(6));
        REQUIRE(cq->push(7));
        REQUIRE(cq->push(8));

        REQUIRE(cq->is_full());
        REQUIRE_FALSE(cq->is_empty());
        REQUIRE(cq->pop(storage));
    }
    SECTION("Normal")
    {
        uint64_t storage{};
        REQUIRE(cq->is_empty());
        REQUIRE_FALSE(cq->pop(storage));

        REQUIRE(cq->push(5));
        REQUIRE(cq->push(6));
        REQUIRE_FALSE(cq->is_full());

        REQUIRE(cq->push(7));
        REQUIRE(cq->is_full());

        REQUIRE_FALSE(cq->push(8));
        REQUIRE(cq->pop(storage));
        REQUIRE(storage == 5);

        REQUIRE(cq->pop(storage));
        REQUIRE(storage == 6);
        REQUIRE_FALSE(cq->is_empty());

        REQUIRE(cq->pop(storage));
        REQUIRE(storage == 7);
        REQUIRE(cq->is_empty());
    }
}
