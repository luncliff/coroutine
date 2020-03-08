/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/linux.h>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto expect_no_resume(epoll_owner& ep, event& efd) -> frame_t {
    co_await wait_in(ep, efd);
    throw std::runtime_error{"unreachable code"};
}

int main(int, char*[]) {
    epoll_owner ep{};
    event e1{};
    // no exception :)
    auto frame = expect_no_resume(ep, e1);
    frame.destroy();
    return EXIT_SUCCESS;
}
