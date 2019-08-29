//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

#include "test.h"
using namespace std;
using namespace coro;

auto spawn_and_join(pthread_t& tid, const pthread_attr_t* attr)
    -> pthread_joiner_t {
    co_await attr;
    tid = pthread_self();
    co_return;
}

auto pthread_joiner_waits_for_join() {
    printf("before spawn: %zu \n", pthread_self());

    pthread_t tid{};
    {
        // joiner's destructor will wait for the new thread to return
        auto joiner = spawn_and_join(tid, nullptr);
    }
    // the new thread's id must be visible at this moment
    printf("after join: %zu \n", tid);

    if (tid == pthread_t{})
        return __LINE__;
    if (tid == pthread_self())
        return __LINE__;

    // it's already joined.
    // so if we trying to join it again, the operation must fail
    if (const auto ec = pthread_join(tid, nullptr); ec != ESRCH) {
        puts(strerror(ec));
        return __LINE__;
    }

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return pthread_joiner_waits_for_join();
}

#endif
