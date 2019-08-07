//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

auto no_spawn_coroutine() -> pthread_joiner_t {
    co_await suspend_never{};
}

auto concrt_pthread_joiner_no_spawn() {
    auto joiner = no_spawn_coroutine();
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return concrt_pthread_joiner_no_spawn();
}

#endif
