//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

auto yield_never() -> enumerable<int> {
    // co_return is necessary so compiler can notice that
    // this is a coroutine when there is no co_yield.
    co_return;
};
auto coro_enumerable_no_yield_test() {
    auto count = 0u;
    for (auto v : yield_never()) {
        v = count; // code to suppress C4189
        count += 1;
    }
    REQUIRE(count == 0);
    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return coro_enumerable_no_yield_test();
}
#endif
