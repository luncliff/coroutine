//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <future>

#include <coroutine/sync.h>

TEST_CASE("MessageTest", "[messaging]")
{
    const thread_id_t main_id = current_thread_id();

    SECTION("SyncWithMessage")
    {
        thread_id_t worker_id{};
        REQUIRE(worker_id == thread_id_t{});

        // some background thread
        //  will send a message to main thread
        auto f = std::async(std::launch::async, [=, &worker_id]() {
            message_t msg{};

            // modify the location
            worker_id = current_thread_id();

            // and then send a message
            post_message(main_id, msg);
        });

        using namespace std::literals;

        message_t msg{};
        // wait for message ...
        while (peek_message(msg) == false)
            continue;

        // after we received a message, the value must be modified
        REQUIRE_FALSE(worker_id == thread_id_t{});

        // no exception from worker
        REQUIRE_NOTHROW(f.get());
    }

    SECTION("DeliveryFromWorkers")
    {
        constexpr auto retry_count = 4;

        auto fwork = [=]() {
            message_t msg{};

            auto repeat = retry_count;
            while (repeat--)
            {
                msg.u64 = static_cast<uint64_t>(current_thread_id());

                // send some messages
                post_message(main_id, msg);

                // zero the message
                msg = message_t{};

                auto max_trial = 10'000;
                while (max_trial--)
                    if (peek_message(msg) == true)
                        // wait for messages for limitied time ...
                        break;

                REQUIRE(msg.u64 != 0);
                REQUIRE(msg.u64 == static_cast<uint64_t>(main_id));
            }
        };

        std::thread t1{fwork};
        std::thread t2{fwork};
        std::thread t3{fwork};
        std::thread t4{fwork};
        std::thread t5{fwork};
        std::thread t6{fwork};
        std::thread t7{fwork};

        // this will be always true, but just check once more
        // REQUIRE(current_thread_id() == main_id);

        size_t count = 0;
        message_t msg{};
        while (count < 7 * retry_count)
            // wait for workers' messages
            if (peek_message(msg) == true)
            {
                // simply check the sender and echo back

                const auto sender_id = static_cast<thread_id_t>(msg.u64);
                REQUIRE(sender_id != main_id);

                msg.u64 = static_cast<uint64_t>(main_id);
                post_message(sender_id, msg);

                // wait for next message ...
                count += 1;
            }

        REQUIRE_NOTHROW(t1.join());
        REQUIRE_NOTHROW(t2.join());
        REQUIRE_NOTHROW(t3.join());
        REQUIRE_NOTHROW(t4.join());
        REQUIRE_NOTHROW(t5.join());
        REQUIRE_NOTHROW(t6.join());
        REQUIRE_NOTHROW(t7.join());
    }
}