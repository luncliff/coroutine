//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/yield.hpp>

#include "test.h"
using namespace std;
using namespace coro;

auto yield_once(int value = 0) -> enumerable<int>;

auto coro_enumerable_after_move_test() {
    auto count = 0u;
    auto g = yield_once();
    auto m = move(g);
    // g lost its handle. so it is not iterable anymore
    _require_(g.begin() == g.end());
    for (auto v : g) {
        v = count; // code to suppress C4189
        _fail_now_("null generator won't go into loop", __FILE__, __LINE__);
    }

    for (auto v : m) {
        _require_(v == 0);
        count += 1;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_enumerable_after_move_test();
}

auto yield_once(int value) -> enumerable<int> {
    co_yield value;
    co_return;
};

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_enumerable_after_move
    : public TestClass<coro_enumerable_after_move> {
    TEST_METHOD(test_coro_enumerable_after_move) {
        coro_enumerable_after_move_test();
    }
};
#endif
