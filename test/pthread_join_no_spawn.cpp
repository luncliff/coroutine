/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/pthread.h>
#include <coroutine/return.h>

using namespace coro;

auto no_spawn_coroutine() -> pthread_joiner {
    co_await suspend_never{};
}

int main(int, char*[]) {
    // joiner's destructor will use `pthread_join`
    // if nothing is spawned, do nothing
    auto joiner = no_spawn_coroutine();
    return EXIT_SUCCESS;
}
