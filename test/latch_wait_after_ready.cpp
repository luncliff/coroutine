//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

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
    _require_(wg.is_ready());

    for (auto& f : contexts)
        f.wait();

    return EXIT_SUCCESS;
};

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return concrt_latch_wait_after_ready_test();
}

#elif __has_include(<CppUnitTest.h>)
class concrt_latch_wait_after_ready
    : public TestClass<concrt_latch_wait_after_ready> {
    TEST_METHOD(test_concrt_latch_wait_after_ready) {
        concrt_latch_wait_after_ready_test();
    }
};
#endif
