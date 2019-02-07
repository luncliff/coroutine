//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <gsl/gsl>

TEST_CASE("return_ignore", "[return]")
{
    // user won't care about coroutine life cycle.
    // the routine will be resumed(continued) properly,
    //   and co_return will destroy the frame
    auto routine = []() -> return_ignore {
        co_await std::experimental::suspend_never{};
        co_return;
    };
    REQUIRE_NOTHROW(routine());
}

TEST_CASE("return_frame", "[return]")
{
    using namespace std::experimental;

    // when the coroutine frame destuction need to be controlled manually,
    //   `return_frame` can do the work
    auto routine = []() -> return_frame {
        // no initial suspend
        co_await suspend_never{};
        co_return;
    };

    // invoke of coroutine will create a new frame
    auto fm = routine();

    // now the frame is 'final suspend'ed, so it can be deleted.
    auto coro = static_cast<coroutine_handle<void>>(fm);
    REQUIRE(coro == true);

    REQUIRE(coro.done());            // 'final suspend'ed?
    REQUIRE_NOTHROW(coro.destroy()); // destroy it
}

TEST_CASE("suspend_hook", "[return]")
{
    using namespace std::experimental;

    suspend_hook hk{};

    SECTION("empty")
    {
        auto coro = static_cast<coroutine_handle<void>>(hk);
        REQUIRE(coro.address() == nullptr);
    }

    SECTION("resume via coroutine handle")
    {
        gsl::index status = 0;

        auto routine = [=](suspend_hook& hook, auto& status) -> return_ignore {
            auto defer = gsl::finally([&]() {
                // ensure final action
                status = 3;
            });

            status = 1;
            co_await suspend_never{};
            co_await hook;
            status = 2;
            co_await hook;
            co_return;
        };

        REQUIRE_NOTHROW(routine(hk, status));
        REQUIRE(status == 1);
        auto coro = static_cast<coroutine_handle<void>>(hk);
        coro.resume();

        REQUIRE(status == 2);
        coro.resume();

        // coroutine reached end.
        // so `defer` in the routine must be destroyed
        REQUIRE(status == 3);
    }

    // test end
}
