//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <future>

#include <coroutine/sync.h>
#include <stop_watch.hpp>

TEST_CASE("message channel", "[messaging]")
{
    using namespace std::chrono;
    using namespace std::literals;

    auto queue_owner = create_message_queue();
    messaging_queue_t* mqueue = queue_owner.get();

    SECTION("wait")
    {
        stop_watch<steady_clock> timer{};

        SECTION("not-empty")
        {
            message_t m{};
            m.u64 = 0xE81F;
            REQUIRE(mqueue->post(m));

            timer.reset();
            REQUIRE(mqueue->wait(m, 100us));

            const auto elapsed = timer.pick<microseconds>();
            REQUIRE(elapsed.count() < 100);
            REQUIRE(m.u64 == 0xE81F);
        }

        SECTION("empty")
        {
            message_t m{};
            timer.reset();
            REQUIRE_FALSE(mqueue->wait(m, 10000us));
            const auto elapsed = timer.pick<microseconds>();

            CAPTURE(elapsed.count());
            REQUIRE(elapsed.count() > 9900); // >= 9.9ms
            REQUIRE(m.u64 == 0);             // zero memory by wait
        }

        SECTION("post while wait")
        {
            auto f1 = std::async(std::launch::async, [=]() {
                stop_watch<system_clock> timer{};
                message_t m{};

                // this should be enough to wait for scheduling of `std::async`
                REQUIRE(mqueue->wait(m, 2ms));
                REQUIRE(m.u64 == 0xE81F);

                const auto elapsed = timer.pick<microseconds>();
                CAPTURE(elapsed.count());
                REQUIRE(elapsed.count() > 0);
                REQUIRE_FALSE(m.u64 == 0); // message must be received
            });
            auto f2 = std::async(std::launch::async, [=]() {
                message_t m{};
                m.u64 = 0xE81F;
                mqueue->post(m);
            });

            REQUIRE_NOTHROW(f1.get());
            REQUIRE_NOTHROW(f2.get());
        }
    }

    SECTION("post and peek")
    {
        message_t m{};

        m.u64 = 0xE81F;
        REQUIRE(mqueue->post(m));
        m.u64 = 0xE820;
        REQUIRE(mqueue->post(m));

        REQUIRE(mqueue->peek(m));
        REQUIRE(m.u64 == 0xE81F);
        REQUIRE(mqueue->peek(m));
        REQUIRE(m.u64 == 0xE820);
    }
}

TEST_CASE("thread messaging", "[thread]")
{
    using namespace std::literals;

    auto queue_owner = create_message_queue();
    messaging_queue_t* mqueue = queue_owner.get();

    SECTION("send message for sync")
    {
        thread_id_t worker_id{};
        REQUIRE(worker_id == thread_id_t{});

        // some background thread
        //  will send a message to main thread
        auto f = std::async(std::launch::async, [=, &worker_id]() {
            // modify the value
            worker_id = current_thread_id();
            // and then send a message
            message_t m{};
            mqueue->post(m);
        });

        message_t m{};
        // this won't take long. just use enough time
        REQUIRE(mqueue->wait(m, 10ms) == true);
        // after we received a message, the value must be modified
        REQUIRE_FALSE(worker_id == thread_id_t{});
        // no exception from worker
        REQUIRE_NOTHROW(f.get());
    }

    SECTION("receive message from multiple senders")
    {
        constexpr auto amount_of_message = 4'000;

        auto fwork = [=]() {
            message_t m{};
            m.u64 = static_cast<uint64_t>(current_thread_id());

            auto repeat = amount_of_message;
            while (repeat--)
                // send some messages
                if (mqueue->post(m) == false)
                    // if failed, try again
                    repeat += 1;
        };

        std::thread //
            t1{fwork},
            t2{fwork}, t3{fwork}, //
            t4{fwork}, t5{fwork}, t6{fwork}, t7{fwork};
        constexpr auto num_worker = 7;

        size_t count = 0;
        message_t m{};
        while (count < num_worker * amount_of_message)
            // wait for workers' messages
            if (mqueue->peek(m) == true)
                // wait for next message ...
                count += 1;

        REQUIRE_NOTHROW(t1.join(), t2.join(), t3.join(), t4.join(), t5.join(),
                        t6.join(), t7.join());

        REQUIRE(count == amount_of_message * num_worker);
    }
}
