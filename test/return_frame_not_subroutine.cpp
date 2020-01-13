//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>
#include <coroutine/return.h>

// this is a coroutine
auto invoke_and_forget_frame() -> coro::frame_t {
    co_await suspend_never{};
    co_return;
};

int main(int, char* []) {
    invoke_and_forget_frame();
    return 0;
}
