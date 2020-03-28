/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */

#undef NDEBUG
#include <cassert>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>

using namespace std;
using namespace coro;
#if defined(__GNUC__)
using no_return_t = coro::null_frame_t;
#else
using no_return_t = std::nullptr_t;
#endif

constexpr int bye = 0;

auto producer(channel<int>& ch) -> no_return_t {
    for (int msg : {1, 2, 3, bye}) {
        auto ok = co_await ch.write(msg);
        // ok == true: we sent a value
        assert(ok);
    }
    puts("producer loop exit");

    // we know that we sent the `bye`,
    // but will send again to check `ok` becomes `false`
    int msg = -1;
    auto ok = co_await ch.write(msg);
    // ok == false: channel is going to be destroyed (no effect for read/write)
    assert(ok == false);
    puts("channel destruction detected");
}

auto consumer_owner() -> no_return_t {
    channel<int> ch{};
    producer(ch); // start a producer routine

    // the type doesn't support for-co_await for now
    for (auto [msg, ok] = co_await ch.read(); ok;
         tie(msg, ok) = co_await ch.read()) {
        // ok == true: we sent a value
        if (msg == bye) {
            puts("consumer loop exit");
            break;
        }
    }
    puts("consumer_owner is going to return");
}

int main(int, char*[]) {
    consumer_owner();
    return EXIT_SUCCESS;
}
