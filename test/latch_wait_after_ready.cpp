//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using latch_t = concrt::latch;

void count_down_on_latch(latch_t* wg) {
    wg->count_down(); // simply count down
}

auto concrt_latch_wait_after_ready_test() {
    static constexpr auto num_of_work = 10u;
    latch_t wg{num_of_work};
    array<future<void>, num_of_work> contexts{};

    // general fork-join will be similar to this ...
    for (auto& f : contexts)
        f = async(launch::async, count_down_on_latch, addressof(wg));

    wg.wait();
    REQUIRE(wg.is_ready());

    for (auto& f : contexts)
        f.wait();

    return EXIT_SUCCESS;
};

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_latch_wait_after_ready_test();
}
#endif
