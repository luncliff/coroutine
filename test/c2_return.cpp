//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>

#include <catch2/catch.hpp>

#include <gsl/gsl>

using namespace coro;
using namespace std::experimental;

// if user doesn't care about coroutine's life cycle,
//  use `no_return`.
// at least the routine will be resumed(continued) properly,
//   `co_return` will destroy the frame
auto invoke_and_forget_frame() -> no_return {
    co_await suspend_never{};
    co_return;
};

TEST_CASE("no_return", "[return]") {
    REQUIRE_NOTHROW(invoke_and_forget_frame());
}

// when the coroutine frame destuction need to be controlled manually,
//  just return `coroutine_handle<void>`.
// `<coroutine/return.h>` supports template specialization of
// `coroutine_traits` for the case. The type implements...
//      return_void
//      initial_suspend  -> suspend_never
//      final_suspend    -> suspend_always
auto invoke_and_get_frame_after_first_suspend() -> frame {
    co_await suspend_never{};
    co_return; // only void return
};

TEST_CASE("get coroutine_handle after first suspend ", "[return]") {
    auto frame = invoke_and_get_frame_after_first_suspend();

    // allow access to `coroutine_handle<void>`
    //  after first suspend(which can be `co_return`)
    coroutine_handle<void>& coro = frame;
    REQUIRE(static_cast<bool>(coro)); // not null
    REQUIRE(coro.done());             // expect final suspended
    REQUIRE_NOTHROW(coro.destroy());  // destroy it
}

auto save_current_handle_to_frame(frame& fh, int& status) -> no_return {
    auto defer = gsl::finally([&]() {
        status = 3; // change state on destruction phase
    });
    status = 1;
    co_await fh; // frame holder is also an awaitable.
    status = 2;
    co_await fh;
    co_return;
}

TEST_CASE("save coroutine_handle to frame object", "[return]") {
    int status = 0;
    frame coro{};

    save_current_handle_to_frame(coro, status);
    REQUIRE(status == 1);

    coro.resume(); // `frame` inherits `coroutine_handle<void>`
    REQUIRE(status == 2);

    coro.resume();        // coroutine reached end.
    REQUIRE(status == 3); // so `defer` in the routine will change status
}
