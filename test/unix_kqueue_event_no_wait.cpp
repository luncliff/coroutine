/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/unix.h>

using namespace coro;

int main(int, char*[]) {
    auto count = 0;
    for (auto task : signaled_event_tasks()) {
        task.resume();
        ++count;
    }
    if (count != 0)
        return __LINE__;

    return EXIT_SUCCESS;
}
