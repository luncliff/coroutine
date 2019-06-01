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
auto coro_enumerable_iterator_test() {
    auto count = 0u;
    auto g = yield_once();
    auto b = g.begin();
    auto e = g.end();
    for (auto it = b; it != e; ++it) {
        REQUIRE(*it == 0);
        count += 1;
    }
    REQUIRE(count > 0);
}

auto coro_enumerable_after_move_test() {
    auto count = 0u;
    auto g = yield_once();
    auto m = move(g);
    // g lost its handle. so it is not iterable anymore
    REQUIRE(g.begin() == g.end());
    for (auto v : g) {
        v = count; // code to suppress C4189
        FAIL_WITH_MESSAGE("null generator won't go into loop"s);
    }

    for (auto v : m) {
        REQUIRE(v == 0);
        count += 1;
    }
    REQUIRE(count > 0);
    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return coro_enumerable_after_move_test();
}
#endif
