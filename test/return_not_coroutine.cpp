/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <cassert>
#include <coroutine/return.h>

// this is not a coroutine
auto invoke_and_no_await() -> coro::frame_t {
    return {};
};

int main(int, char*[]) {
    invoke_and_no_await();
    return 0;
}
