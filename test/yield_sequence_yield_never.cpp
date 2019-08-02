//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace coro;
using status_t = int64_t;

auto sequence_yield_never() -> sequence<status_t> {
    co_return;
}
auto use_sequence_yield_never(status_t& ref) -> frame {
    // clang-format off
    for co_await(auto v : sequence_yield_never())
        ref = v;
    // clang-format on
};

auto coro_sequence_yield_never_test() {
    frame f1{}; // frame holder for the caller
    auto on_return = gsl::finally([&f1]() {
        if (f1.address() != nullptr)
            f1.destroy();
    });

    status_t storage = -1;
    // since there was no yield, it will remain unchanged
    f1 = use_sequence_yield_never(storage);
    _require_(storage == -1);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_sequence_yield_never_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_sequence_yield_never : public TestClass<coro_sequence_yield_never> {
    TEST_METHOD(test_coro_sequence_yield_never) {
        coro_sequence_yield_never_test();
    }
};
#endif