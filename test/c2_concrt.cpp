//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>

#include <catch2/catch.hpp>

#include <array>
#include <future>
#include <queue>

using namespace std;
using namespace literals;
using namespace std::experimental;

TEST_CASE("latch", "[concurrency][thread]") {
    using wait_group = concrt::latch; // the type's name in Go Language

    SECTION("ready after wait") {
        constexpr auto num_of_work = 10u;
        array<future<void>, num_of_work> contexts{};

        wait_group group{num_of_work};

        // fork - join
        for (auto& fut : contexts)
            fut = async(launch::async,
                        [](wait_group* group) -> void {
                            // simply count down
                            group->count_down();
                        },
                        addressof(group));
        group.wait();
        REQUIRE(group.is_ready());

        for (auto& fut : contexts)
            fut.wait();
    }

    SECTION("count down and wait") {
        wait_group group{1};
        REQUIRE_FALSE(group.is_ready());

        group.count_down_and_wait();
        REQUIRE(group.is_ready());
        REQUIRE(group.is_ready()); // mutliple test is ok
    }

    SECTION("throws for negative") {
        SECTION("from positive") {
            wait_group group{1};
            REQUIRE_THROWS(group.count_down(4));
        }
        SECTION("from zero") {
            wait_group group{0};
            REQUIRE_THROWS(group.count_down(2));
        }
    }
}

using namespace coro;

TEST_CASE("frame holder is coroutine_handle", "[return]") {
    frame fh{};
    auto coro = static_cast<coroutine_handle<void>>(fh);
    REQUIRE(coro.address() == nullptr);
}

auto wait_push(queue<coroutine_handle<void>>& q) noexcept {
    using queue_type = queue<coroutine_handle<void>>;

    struct push_redirect : suspend_always {
        queue_type& q;

      public:
        explicit push_redirect(queue_type& _q) noexcept : q{_q} {};
        ~push_redirect() noexcept = default;

        void await_suspend(coroutine_handle<void> coro) noexcept(false) {
            q.push(coro); // push current coroutine into the queue
        }
    };
    return push_redirect{q};
}

auto wait_pop(queue<coroutine_handle<void>>& q) noexcept {
    using queue_type = queue<coroutine_handle<void>>;

    struct pop_redirect : suspend_never {
        queue_type& q;

      public:
        explicit pop_redirect(queue_type& _q) noexcept : q{_q} {};
        ~pop_redirect() noexcept = default;

        auto await_resume() noexcept(false) {
            coroutine_handle<void> coro{};
            if (q.empty() == false) {
                coro = q.front();
                q.pop();
            }
            return coro;
        }
    };
    return pop_redirect{q};
}

template <typename T>
auto push_using_await(queue<T>& queue) -> no_return {
    co_await wait_push(queue);
}
template <typename T>
auto pop_using_await(queue<T>& queue, T& item) -> no_return {
    item = co_await wait_pop(queue);
}

TEST_CASE("coroutine and queue", "[await]") {
    using value_type = coroutine_handle<void>;
    queue<value_type> queue{};

    SECTION("pop on empty") {
        value_type coro{};
        pop_using_await(queue, coro);
        REQUIRE(coro.address() == nullptr);

        push_using_await(queue);
    }
    SECTION("pop after push") {
        push_using_await(queue);

        value_type coro{};
        pop_using_await(queue, coro);
        REQUIRE(static_cast<bool>(coro));
        // since `push_using_await` is suspended state, we can resume it
        REQUIRE_NOTHROW(coro.resume());
        // `push_using_await` returns `no_return`.
        //  so it doesn't need to be destroyed explicitly
    }
}
