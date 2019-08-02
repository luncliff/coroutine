//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/yield.hpp>

#include "test.h"
using namespace std;
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
    _require_(count == 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_enumerable_no_yield_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_enumerable_no_yield : public TestClass<coro_enumerable_no_yield> {
    TEST_METHOD(test_coro_enumerable_no_yield) {
        coro_enumerable_no_yield_test();
    }
};
#endif
