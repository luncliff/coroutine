//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <concurrency_helper.h>
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

auto concrt_latch_wait_multiple_test() {

    latch wg{1};
    _require_(wg.is_ready() == false);

    wg.count_down_and_wait();
    _require_(wg.is_ready());
    _require_(wg.is_ready()); // mutliple test is ok

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return concrt_latch_wait_multiple_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class concrt_latch_wait_multiple
    : public TestClass<concrt_latch_wait_multiple> {
    TEST_METHOD(test_concrt_latch_wait_multiple) {
        concrt_latch_wait_multiple_test();
    }
};
#endif
