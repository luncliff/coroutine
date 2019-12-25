//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/yield.hpp>

#include "test.h"
using namespace std;
using namespace coro;

auto yield_rvalue() -> enumerable<string> {
    co_yield "1";
    co_yield "2";
    co_yield "3";
};

auto coro_enumerable_rvalue_test() {
    string txt{};
    for (auto v : yield_rvalue()) {
        txt += v;
        txt += ",";
    }
    _require_(txt == "1,2,3,");
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_enumerable_rvalue_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_enumerable_rvalue : public TestClass<coro_enumerable_rvalue> {
    TEST_METHOD(test_coro_enumerable_rvalue) {
        coro_enumerable_rvalue_test();
    }
};
#endif
