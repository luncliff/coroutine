/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/pthread.h>
#include <coroutine/return.h>
#include <future>

using namespace coro;

auto multiple_spawn_coroutine(std::promise<void>& p, const pthread_attr_t* attr)
    -> pthread_detacher_t {
    try {
        co_await attr;
        co_await attr; // can't spawn mutliple times

        p.set_value();
    } catch (const logic_error& e) {
        p.set_exception(std::current_exception());
    }
}

int main(int, char*[]) {
    std::promise<void> report{};
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
