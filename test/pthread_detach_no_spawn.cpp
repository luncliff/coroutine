/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/pthread.h>
#include <coroutine/return.h>

using namespace coro;

auto no_spawn_coroutine() -> pthread_detacher {
    co_await suspend_never{};
}

int main(int, char*[]) {
    auto detacher = no_spawn_coroutine();

    // we didn't created the thread. this must be the zero
    pthread_t tid = detacher;
    if (tid != pthread_t{})
        return __LINE__;

    return EXIT_SUCCESS;
}
