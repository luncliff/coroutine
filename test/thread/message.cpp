//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include <future>

#include <coroutine/sync.h>

TEST_CASE("message operations", "[messaging]") {
  using namespace std::literals;

  const thread_id_t main_id = current_thread_id();

  SECTION("sync with message") {

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

  SECTION("merge") {
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
