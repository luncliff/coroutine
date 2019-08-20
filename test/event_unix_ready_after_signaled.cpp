//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>

using namespace coro;

auto concrt_event_ready_after_signaled_test() {
    auto_reset_event e1{};
    e1.set(); // when the event is signaled,
              // `co_await` on it must proceed without suspend
    if (e1.await_ready() == false)
        return __LINE__;

    return EXIT_SUCCESS;
}

#if !__has_include(<CppUnitTest.h>)
int main(int, char*[]) {
    return concrt_event_ready_after_signaled_test();
}
#endif
