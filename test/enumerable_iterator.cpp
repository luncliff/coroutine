//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace coro;

auto yield_once(int value = 0) -> enumerable<int>;

auto coro_enumerable_iterator_test() {
    auto count = 0u;
    auto g = yield_once();
    auto b = g.begin();
    auto e = g.end();
    for (auto it = b; it != e; ++it) {
        _require_(*it == 0);
        count += 1;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_enumerable_iterator_test();
}

auto yield_once(int value) -> enumerable<int> {
    co_yield value;
    co_return;
};

#elif __has_include(<CppUnitTest.h>)
class coro_enumerable_iterator : public TestClass<coro_enumerable_iterator> {
    TEST_METHOD(test_coro_enumerable_iterator) {
        coro_enumerable_iterator_test();
    }
};

#endif
