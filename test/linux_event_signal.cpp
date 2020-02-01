/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/unix.h>

using namespace coro;

int main(int, char*[]) {
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
