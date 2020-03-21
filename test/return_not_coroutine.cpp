/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <coroutine/return.h>

// this is not a coroutine
auto invoke_and_no_await() -> coro::frame_t {
    return coro::frame_t{};
};

int main(int, char* []) {
    auto frame = invoke_and_no_await();
    assert(frame.address() == nullptr);
    assert(static_cast<bool>(frame) == false);
    return 0;
}
