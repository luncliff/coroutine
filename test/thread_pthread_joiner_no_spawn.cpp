//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto no_spawn_coroutine() -> pthread_joiner_t {
    co_await suspend_never{};
}

auto pthread_joiner_no_spawn() {
    // joiner's destructor will use `pthread_join`
    // if nothing is spawned, do nothing
    auto joiner = no_spawn_coroutine();

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return pthread_joiner_no_spawn();
}

#endif
