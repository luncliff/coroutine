/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <mutex>

#include "internal/channel.hpp"
#include <coroutine/return.h> // includes `coroutine_traits<void, ...>`

using namespace std;
using namespace coro;

using channel_with_lock_t = channel<int, mutex>;

auto write_to(channel_with_lock_t& ch, int value, bool ok = false) -> void {
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

auto read_from(channel_with_lock_t& ch, int& ref, bool ok = false) -> void {
    tie(ref, ok) = co_await ch.read();
    assert(ok);
}

int main(int, char*[]) {
    const auto list = {1, 2, 3};
    channel_with_lock_t ch{};
    int storage = 0;

    for (auto i : list) {
        write_to(ch, i);      // Writer coroutine will suspend
        assert(storage != i); // so no write occurs
    }
    for (auto i : list) {
        read_from(ch, storage); // read to `storage`
        assert(storage == i);   // stored value is same with sent value
    }

    return EXIT_SUCCESS;
}
