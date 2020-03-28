/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <coroutine/return.h>

#if defined(__GNUC__)
using no_return_t = coro::null_frame_t;
#else
using no_return_t = std::nullptr_t;
#endif

// this is a coroutine
auto invoke_and_forget_frame() -> no_return_t {
    co_await std::suspend_never{};
    co_return;
};

int main(int, char*[]) {
    invoke_and_forget_frame();
    return 0;
}
