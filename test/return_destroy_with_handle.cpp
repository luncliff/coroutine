/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <gsl/gsl>

#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto save_current_handle_to_frame(int& status) -> frame_t {
    auto on_return = gsl::finally([&status]() {
        // change the status when the frame is going to be destroyed
        status += 1;
    });

    status += 1;
    co_await suspend_always{};
    status += 1;
}

int main(int, char*[]) {
    int status = 0;
    coroutine_handle<void> coro = save_current_handle_to_frame(status);
    assert(status == 1);
    assert(coro.address() != nullptr);
    coro.resume();

    // the coroutine reached end
    // so, `gsl::finally` should changed the status
    assert(coro.done());
    assert(status == 3);

    coro.destroy();
    return 0;
}
