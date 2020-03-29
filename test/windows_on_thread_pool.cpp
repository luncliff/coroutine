/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <atomic>
#include <cassert>
#include <iostream>

#include <gsl/gsl>

#include <coroutine/return.h>
#include <coroutine/windows.h>

#include <latch.h>

using namespace std;
using namespace coro;

auto change_and_report(std::latch& wg, atomic_uint32_t& counter) -> frame_t {
    co_await continue_on_thread_pool{};
    --counter;
    wg.count_down();
}

int main(int, char*[]) {
    constexpr uint32_t num_worker = 40u;

    atomic_uint32_t counter = num_worker;
    std::latch wg{static_cast<ptrdiff_t>(num_worker)};
    for (auto i = 0u; i < num_worker; ++i) { // fork
        change_and_report(wg, counter);
    }
    wg.wait(); // join

    assert(counter == 0);
    return EXIT_SUCCESS;
}
