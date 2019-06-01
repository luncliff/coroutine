//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using status_t = int64_t;

auto sequence_suspend_with_yield(frame& manual_resume) -> sequence<status_t> {
    status_t value = 0;
    co_yield value = 1;
    co_yield manual_resume; // use `co_yield` instead of `co_await`
    co_yield value = 2;
};
auto use_sequence_yield_suspend_yield_2(status_t& ref, frame& fh) -> frame {
    // clang-format off
    for co_await(auto v : sequence_suspend_with_yield(fh))
        ref = v;
    // clang-format on
};

auto coro_sequence_suspend_using_yield_test() {
    // see also: `coro_sequence_frame_status_test`
    frame fs{};
    status_t storage = -1;
    auto fc = use_sequence_yield_suspend_yield_2(storage, fs);
    auto on_return = gsl::finally([&fc]() {
        if (fc)
            fc.destroy();
    });

    REQUIRE(fs.done() == false); // we didn't finished iteration
    REQUIRE(storage == 1);       // co_yield value = 1
    // continue after co_await manual_resume;
    fs.resume();

    REQUIRE(storage == 2); // co_yield value = 2;
    REQUIRE(fs.done() == true);
    REQUIRE(fc.done() == true);

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return coro_sequence_suspend_using_yield_test();
}
#endif
