//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/yield.hpp>

#include "test.h"
using namespace coro;

auto yield_once(int value = 0) -> enumerable<int> {
    co_yield value;
    co_return;
};
auto coro_enumerable_yield_once_test() {
    auto count = 0u;
    for (auto v : yield_once()) {
        _require_(v == 0);
        count += 1;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_enumerable_yield_once_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_enumerable_yield_once
    : public TestClass<coro_enumerable_yield_once> {
    TEST_METHOD(test_coro_enumerable_yield_once) {
        coro_enumerable_yield_once_test();
    }
};
#endif
