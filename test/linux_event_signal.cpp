/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <array>

#include <coroutine/linux.h>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto mark_after_resume(epoll_owner& ep, event& event, //
                       bool& resumed) -> frame_t {
    co_await wait_in(ep, event);
    resumed = true;
}

int main(int, char*[]) {
    epoll_owner ep{};
    event e1{};
    bool resumed = false;
    mark_after_resume(ep, e1, resumed);

    e1.set();

    array<epoll_event, 1> events{};
    auto count = ep.wait(1000, events);
    if (count == 0)
        return __LINE__;

    auto coro = coroutine_handle<void>::from_address(events[0].data.ptr);
    coro.resume();
    if (resumed == false)
        return __LINE__;

    coro.destroy();
    return EXIT_SUCCESS;
}
