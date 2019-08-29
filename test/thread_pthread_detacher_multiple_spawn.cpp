//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

#include "test.h"
#include <future>
using namespace std;
using namespace coro;

auto multiple_spawn_coroutine(promise<void>& p, const pthread_attr_t* attr)
    -> pthread_detacher_t {
    try {
        co_await attr;
        co_await attr; // can't spawn mutliple times

        p.set_value();
    } catch (const logic_error& e) {
        p.set_exception(std::current_exception());
    }
}

auto pthread_detacher_throws_multiple_spawn() {

    promise<void> report{};
    {
        // detacher uses `pthread_detach` in its destructor.
        auto detacher = multiple_spawn_coroutine(report, nullptr);
    }

    auto signal = report.get_future();
    signal.wait();

    try {
        signal.get();
        return EXIT_FAILURE;
    } catch (...) {
        // we expect an exceptiion form the bad code(multiple spawn)
        return EXIT_SUCCESS;
    }
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return pthread_detacher_throws_multiple_spawn();
}

#endif
