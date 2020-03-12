/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */

#undef NDEBUG
#include <cassert>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

using channel_without_lock_t = channel<int>;

int main(int, char*[]) {
    auto ch = make_unique<channel_without_lock_t>();
    bool ok = true;

    auto coro_read = [&ok](auto& ch, auto& value) -> frame_t {
        tie(value, ok) = co_await ch.read();
    };

    auto item = int{};
    // coroutine will suspend and wait in the channel
    auto h = coro_read(*ch, item);
    {
        auto truncator = move(ch); // if channel is destroyed ...
    }
    assert(ch.get() == nullptr);

    coroutine_handle<void>& coro = h;
    assert(coro.done()); // coroutine is in done state
    coro.destroy();      // destroy to prevent leak

    assert(ok == false); // and channel returned false
    return EXIT_SUCCESS;
}
