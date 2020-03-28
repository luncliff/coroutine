/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */

#undef NDEBUG
#include <cassert>
#include <mutex>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

using channel_with_lock_t = channel<int, mutex>;
#if defined(__GNUC__)
using no_return_t = coro::null_frame_t;
#else
using no_return_t = std::nullptr_t;
#endif

auto write_to(channel_with_lock_t& ch, int value, bool ok = false)
    -> no_return_t {
    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like forget_frameimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;
    assert(ok);
}

auto read_from(channel_with_lock_t& ch, int& ref, bool ok = false)
    -> no_return_t {
    tie(ref, ok) = co_await ch.read();
    assert(ok);
}

int main(int, char*[]) {
    const auto list = {1, 2, 3};
    channel_with_lock_t ch{};
    int storage = 0;

    for (auto i : list) {
        // Reader coroutine will suspend
        read_from(ch, storage);
        // so no effect for the read
        assert(storage != i);
    }
    for (auto i : list) {
        // writer will send a value
        write_to(ch, i);
        // stored value is same with sent value
        assert(storage == i);
    }
    return EXIT_SUCCESS;
}
