//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch.hpp>

#include <future>

#include <coroutine/sync.h>

TEST_CASE("message operations", "[messaging]")
{
    using namespace std::literals;
    const thread_id_t main_id = current_thread_id();

    SECTION("send message to itself")
    {
        message_t msg{};
        msg.u64 = 0xE81F;
        REQUIRE(post_message(current_thread_id(), msg));
        msg.u64 = 0xE820;
        REQUIRE(post_message(current_thread_id(), msg));

        REQUIRE(peek_message(msg)); // receive in order
        REQUIRE(msg.u64 == 0xE81F);
        REQUIRE(peek_message(msg));
        REQUIRE(msg.u64 == 0xE820);
    }

    SECTION("send message to unknown throws")
    {
        // since there is no thread with id 0,
        // this will be the id of unknown
        thread_id_t tid{};

        message_t msg{};
        // throws exception
        REQUIRE_THROWS(post_message(tid, msg));
        // REQUIRE_THROWS_AS(post_message(tid, msg), std::exception);
    }

    SECTION("send message to unregisterd threads")
    {
        auto fwork = [&]() {
            // sleep and do nothing
            std::this_thread::sleep_for(2s);
        };

        std::thread t1{fwork}, t2{fwork};

        message_t msg{};
        // this is a valid id, but it is not registered
        auto std_id = t1.get_id();

#if __APPLE__ || __linux__ || __unix__
        static_assert(sizeof(thread_id_t) == sizeof(decltype(t1.get_id())));
        auto tid = *reinterpret_cast<thread_id_t*>(std::addressof(std_id));
#elif _MSC_VER
        static_assert(sizeof(uint32_t) == sizeof(decltype(t1.get_id())));
        auto tid = static_cast<thread_id_t>(
            *reinterpret_cast<uint32_t*>(std::addressof(std_id)));
#endif

        REQUIRE(tid != thread_id_t{});
        // if the thread id is valid,
        //  library will buffer the message
        REQUIRE(post_message(tid, msg) == true);

        std_id = t2.get_id();
#if __APPLE__ || __linux__ || __unix__
        tid = *reinterpret_cast<thread_id_t*>(std::addressof(std_id));
#elif _MSC_VER
        tid = static_cast<thread_id_t>(
            *reinterpret_cast<uint32_t*>(std::addressof(std_id)));
#endif
        REQUIRE(tid != thread_id_t{});
        REQUIRE(post_message(tid, msg) == true);

        REQUIRE_NOTHROW(t1.join());
        REQUIRE_NOTHROW(t2.join());
    }

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
            message_t msg{};
            post_message(main_id, msg);
        });

        message_t msg{};
        // wait for message ...
        while (peek_message(msg) == false)
            continue;

        // after we received a message, the value must be modified
        REQUIRE_FALSE(worker_id == thread_id_t{});

        // no exception from worker
        REQUIRE_NOTHROW(f.get());
    }

    SECTION("receive message from multiple senders")
    {
        constexpr auto amount_of_message = 4'000;

        auto fwork = [=]() {
            message_t msg{};
            msg.u64 = static_cast<uint64_t>(current_thread_id());

            auto repeat = amount_of_message;
            while (repeat--)
                // send some messages
                if (post_message(main_id, msg) == false)
                    // if failed, try again
                    repeat += 1;
        };

        std::thread //
            t1{fwork},
            t2{fwork}, t3{fwork}, //
            t4{fwork}, t5{fwork}, t6{fwork}, t7{fwork};
        constexpr auto num_worker = 7;

        size_t count = 0;
        message_t msg{};
        while (count < num_worker * amount_of_message)
            // wait for workers' messages
            if (peek_message(msg) == true)
                // wait for next message ...
                count += 1;

        REQUIRE_NOTHROW(t1.join());
        REQUIRE_NOTHROW(t2.join());
        REQUIRE_NOTHROW(t3.join());
        REQUIRE_NOTHROW(t4.join());
        REQUIRE_NOTHROW(t5.join());
        REQUIRE_NOTHROW(t6.join());
        REQUIRE_NOTHROW(t7.join());

        REQUIRE(count == amount_of_message * num_worker);
    }
}
