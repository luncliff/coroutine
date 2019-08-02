//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

using channel_without_lock_t = channel<int>;

template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> forget_frame {
    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like clang optimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;
    _require_(ok);
}

template <typename E, typename L>
auto read_from(channel<E, L>& ch, E& value, bool ok = false) -> forget_frame {
    tie(value, ok) = co_await ch.read();
    _require_(ok);
}

auto coro_channel_write_before_read_test() {
    const auto list = {1, 2, 3};
    channel_without_lock_t ch{};
    int storage = 0;

    for (auto i : list) {
        write_to(ch, i);         // Writer coroutine will suspend
        _require_(storage != i); // so no write occurs
    }
    for (auto i : list) {
        read_from(ch, storage);  // read to `storage`
        _require_(storage == i); // stored value is same with sent value
    }

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_channel_write_before_read_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_channel_write_before_read
    : public TestClass<coro_channel_write_before_read> {
    TEST_METHOD(test_coro_channel_write_before_read) {
        coro_channel_write_before_read_test();
    }
};
#endif
