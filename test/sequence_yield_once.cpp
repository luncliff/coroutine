//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using status_t = int64_t;

// use like a generator
auto sequence_yield_once(status_t value = 0) -> sequence<status_t> {
    co_yield value;
    co_return;
}
auto use_sequence_yield_once(status_t& ref) -> no_return {
    // clang-format off
    for co_await(auto v : sequence_yield_once(0xBEE))
        ref = v;
    // clang-format on
};

auto coro_sequence_yield_once_test() {
    status_t storage = -1;
    use_sequence_yield_once(storage);
    REQUIRE(storage == 0xBEE); // storage will receive value

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_sequence_yield_once_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_sequence_yield_once : public TestClass<coro_sequence_yield_once> {
    TEST_METHOD(test_coro_sequence_yield_once) {
        coro_sequence_yield_once_test();
    }
};
#endif
