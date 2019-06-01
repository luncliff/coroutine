//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using status_t = int64_t;

auto sequence_suspend_with_await(frame& manual_resume) -> sequence<status_t> {
    status_t value = 0;
    co_yield value = 1;
    co_await manual_resume; // use `co_await` instead of `co_yield`
    co_yield value = 2;
}
auto use_sequence_yield_suspend_yield_1(status_t& ref, frame& fh) -> frame {
    // clang-format off
    for co_await(auto v : sequence_suspend_with_await(fh))
        ref = v;
    // clang-format on
};

auto coro_sequence_suspend_using_await_test() {
    // see also: `coro_sequence_frame_status_test`
    frame fs{};
    status_t storage = -1;
    auto fc = use_sequence_yield_suspend_yield_1(storage, fs);
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

#if __has_include(<CppUnitTest.h>)
class coro_sequence_suspend_using_await
    : public TestClass<coro_sequence_suspend_using_await> {
    TEST_METHOD(test_coro_sequence_suspend_using_await) {
        coro_sequence_suspend_using_await_test();
    }
};
#else
int main(int, char* []) {
    return coro_sequence_suspend_using_await_test();
}
#endif
