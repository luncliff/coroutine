//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

using u32_chan_t = channel<uint32_t>;
using i32_chan_t = channel<int32_t>;

template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> no_return {
    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like clang optimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;
    REQUIRE(ok);
}

auto coro_channel_select_peek_every_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    write_to(ch1, 17u);
    write_to(ch2, 15);

    select(ch2, [](auto v) { REQUIRE(v == 15); }, //
           ch1, [](auto v) { REQUIRE(v == 17u); } //
    );

    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class coro_channel_select_peek_every
    : public TestClass<coro_channel_select_peek_every> {
    TEST_METHOD(test_coro_channel_select_peek_every) {
        coro_channel_select_peek_every_test();
    }
};
#else
int main(int, char* []) {
    return coro_channel_select_peek_every_test();
}
#endif
