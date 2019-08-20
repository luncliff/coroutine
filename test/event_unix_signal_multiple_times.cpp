//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>

using namespace coro;

auto concrt_event_signal_multiple_test() {
    auto_reset_event e1{};

    e1.set();
    if (e1.await_ready() == false)
        return __LINE__;

    e1.set();
    e1.set();
    if (e1.await_ready() == false)
        return __LINE__;

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_signal_multiple_test();
}
#endif
