//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

auto yield_once(int value = 0) -> enumerable<int> {
    co_yield value;
    co_return;
};
auto coro_enumerable_yield_once_test() {

    auto count = 0u;
    for (auto v : yield_once()) {
        REQUIRE(v == 0);
        count += 1;
    }
    REQUIRE(count > 0);
    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return coro_enumerable_yield_once_test();
}
#endif
