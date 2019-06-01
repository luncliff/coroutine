//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto get_threads_test() {
    auto pid = GetCurrentProcessId();
    bool detected = false;
    // it's simple. find peer threads
    for (auto tid : get_threads(pid)) {
        if (tid == GetCurrentThreadId())
            continue;
        detected = true;
    }
    // since visual studio native test runs with multi-thread,
    // there must be a peer thread for this test process
    REQUIRE(detected);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return get_threads_test();
}

#elif __has_include(<CppUnitTest.h>)
class win32_get_threads : public TestClass<win32_get_threads> {
    TEST_METHOD(test_get_threads) {
        get_threads_test();
    }
};
#endif
