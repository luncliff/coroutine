/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <cassert>
#include <coroutine/return.h>

using namespace coro;

// this is a coroutine
auto invoke_and_forget_frame() -> coro::frame_t {
    co_await suspend_never{};
    co_return;
};

int main(int, char*[]) {
    invoke_and_forget_frame();
    return 0;
}
