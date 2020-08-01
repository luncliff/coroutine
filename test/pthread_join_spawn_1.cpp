/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <atomic>
#include <coroutine/pthread.h>
#include <coroutine/return.h>

using namespace coro;
using namespace std;

auto multiple_spawn_coroutine(atomic_bool& throwed, const pthread_attr_t* attr)
    -> pthread_joiner {
    try {
        co_await attr;
        co_await attr; // can't spawn mutliple times
    } catch (const logic_error& e) {
        throwed = true;
    }
}

int main(int, char*[]) {
    atomic_bool throwed = false;
    {
        // ensure join before test
        auto join = multiple_spawn_coroutine(throwed, nullptr);
    }

    if (throwed == false)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
