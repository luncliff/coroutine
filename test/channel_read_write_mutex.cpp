//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include "test.h"
using namespace coro;

using channel_with_lock_t = channel<int, mutex>;

template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> forget_frame {
    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like forget_frameimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;
    _require_(ok, __FILE__, __LINE__);
}

template <typename E, typename L>
auto read_from(channel<E, L>& ch, E& value, bool ok = false) -> forget_frame {
    tie(value, ok) = co_await ch.read();
    _require_(ok, __FILE__, __LINE__);
}

auto coro_channel_mutexed_read_before_write_test() {
    const auto list = {1, 2, 3};
    channel_with_lock_t ch{};
    int storage = 0;

    for (auto i : list) {
        // Reader coroutine will suspend
        read_from(ch, storage);
        // so no effect for the read
        _require_(storage != i, __FILE__, __LINE__);
    }
    for (auto i : list) {
        // writer will send a value
        write_to(ch, i);
        // stored value is same with sent value
        _require_(storage == i, __FILE__, __LINE__);
    }

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_channel_mutexed_read_before_write_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_channel_mutexed_read_before_write
    : public TestClass<coro_channel_mutexed_read_before_write> {
    TEST_METHOD(test_coro_channel_mutexed_read_before_write) {
        coro_channel_mutexed_read_before_write_test();
    }
};

#endif
