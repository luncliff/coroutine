/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <atomic>
#include <chrono>
#include <thread>

#include <coroutine/pthread.h>
#include <coroutine/return.h>

using namespace coro;

auto multiple_spawn_coroutine(std::atomic_bool& p, const pthread_attr_t* attr)
    -> pthread_detacher {
    try {
        co_await attr;
        co_await attr; // can't spawn mutliple times

        p = false; // unreachable code
    } catch (const std::logic_error& e) {
        fprintf(stderr, "%s %lx \n", __func__, attr);
        p = true;
    }
}

int main(int, char*[]) {
    std::atomic_bool errored = false;
    {
        // detacher uses `pthread_detach` in its destructor.
        auto _ = multiple_spawn_coroutine(errored, nullptr);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    // we expect an exceptiion form the bad code(multiple spawn)
    return errored ? EXIT_SUCCESS : EXIT_FAILURE;
}
