//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto no_spawn_coroutine() -> pthread_detacher_t {
    co_await suspend_never{};
}

auto pthread_detacher_no_spawn() {
    auto detacher = no_spawn_coroutine();

    // we didn't created the thread. this must be the zero
    pthread_t tid = detacher;
    if (tid != pthread_t{})
        return __LINE__;

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return pthread_detacher_no_spawn();
}

#endif
