//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/return.h>
#include <gsl/gsl>

using namespace coro;
using namespace std::experimental;

TEST_CASE("return_ignore", "[return]")
{
    // user won't care about coroutine life cycle.
    // the routine will be resumed(continued) properly,
    //   and co_return will destroy the frame
    auto example = []() -> return_ignore {
        co_await suspend_never{};
        co_return;
    };
    REQUIRE_NOTHROW(example());
}

TEST_CASE("return_frame", "[return]")
{

    // when the coroutine frame destuction need to be controlled manually,
    //   `return_frame` can do the work
    //   it returns `suspend_always` for `final_suspend`
    auto example = []() -> return_frame {
        // no initial suspend for return_frame
        co_await suspend_never{};
        co_return;
    };

    // invoke of coroutine will create a new frame holder
    auto h = example();

    // now the frame is 'final suspend'ed, so it can be deleted.
    auto coro = h.get();
    REQUIRE(coro == true);
    REQUIRE(coro.done());            // 'final suspend'ed?
    REQUIRE_NOTHROW(coro.destroy()); // destroy it
}
