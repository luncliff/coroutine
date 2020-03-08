/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <array>

#include <coroutine/linux.h>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto wait_for_multiple_times(epoll_owner& ep, event& efd, //
                             uint32_t counter) -> frame_t {
    while (counter--)
        co_await wait_in(ep, efd);
}

void resume_signaled_tasks(epoll_owner& ep) {
    array<epoll_event, 10> events{};
    auto count = ep.wait(1000, events); // wait for 1 sec
    if (count == 0)
        return;

    for_each(events.begin(), events.begin() + count, [](epoll_event& e) {
        auto coro = coroutine_handle<void>::from_address(e.data.ptr);
        coro.resume();
    });
}

int main(int, char*[]) {
    epoll_owner ep{};
    event e1{};

    wait_for_multiple_times(ep, e1, 6); // 6 await
    auto repeat = 8u;                   // + 2 timeout
    while (repeat--) {
        e1.set();
        // resume if there is available event-waiting coroutines
        resume_signaled_tasks(ep);
    };
    return EXIT_SUCCESS;
}
