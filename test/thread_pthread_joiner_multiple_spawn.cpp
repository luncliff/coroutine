//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto multiple_spawn_coroutine(atomic_bool& throwed, const pthread_attr_t* attr)
    -> pthread_joiner_t {
    try {
        co_await attr;
        co_await attr; // can't spawn mutliple times
    } catch (const logic_error& e) {
        throwed = true;
    }
}

auto pthread_joiner_throws_multiple_spawn() {
    atomic_bool throwed = false;
    {
        // ensure join before test
        auto join = multiple_spawn_coroutine(throwed, nullptr);
    }

    if (throwed == false)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return pthread_joiner_throws_multiple_spawn();
}

#endif
