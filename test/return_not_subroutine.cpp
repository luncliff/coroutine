/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <coroutine/return.h>

// this is a coroutine
auto invoke_and_forget_frame() -> std::nullptr_t {
    co_await std::suspend_never{};
    co_return;
};

int main(int, char* []) {
    invoke_and_forget_frame();
    return 0;
}
