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
