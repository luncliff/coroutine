//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

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
auto coro_frame_awaitable_test() {
    int status = 0;
    frame coro{};

    save_current_handle_to_frame(coro, status);
    REQUIRE(status == 1);

    // `frame` inherits `coroutine_handle<void>`
    coro.resume();
    REQUIRE(status == 2);

    // coroutine reached end.
    // so `defer` in the routine will change status
    coro.resume();
    REQUIRE(status == 3);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return coro_frame_awaitable_test();
}
#endif
