//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

using u32_chan_t = channel<uint32_t>;
using i32_chan_t = channel<int32_t>;

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

auto coro_channel_select_peek_every_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    write_to(ch1, 17u);
    write_to(ch2, 15);

    select(ch2, [](auto v) { _require_(v == 15); }, //
           ch1, [](auto v) { _require_(v == 17u); } //
    );

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_channel_select_peek_every_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_channel_select_peek_every
    : public TestClass<coro_channel_select_peek_every> {
    TEST_METHOD(test_coro_channel_select_peek_every) {
        coro_channel_select_peek_every_test();
    }
};
#endif
