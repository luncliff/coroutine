/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <atomic>
#include <cassert>
#include <iostream>

#include <gsl/gsl>

#include <coroutine/return.h>
#include <coroutine/windows.h>

using namespace std;
using namespace coro;

auto decrease_async(atomic_uint32_t& counter) -> frame_t {
    co_await continue_on_thread_pool{};
    --counter;
}

int main(int, char*[]) {
    constexpr auto num_worker = 40u;
    // need a latch here ...
    atomic_uint32_t counter = num_worker;
    for (auto i = 0; i < num_worker; ++i) { // fork
        decrease_async(counter);
    }
    // wg.wait(); // join
    return EXIT_SUCCESS;
}
