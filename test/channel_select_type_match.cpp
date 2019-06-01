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

auto coro_channel_select_type_matching_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};
    write_to(ch1, 17u);

    // the callback functions can be a coroutine (which includes
    // subroutine), but their returns will be truncated !
    select(ch2,
           [](auto v) {
               static_assert(is_same_v<decltype(v), int32_t>);
               FAIL_WITH_MESSAGE("select on empty channel must bypass"s);
           },
           ch1,
           [](auto v) -> no_return {
               static_assert(is_same_v<decltype(v), uint32_t>);
               REQUIRE(v == 17u);

               co_await suspend_never{};
           });

    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class coro_channel_select_type_matching
    : public TestClass<coro_channel_select_type_matching> {
    TEST_METHOD(test_coro_channel_select_type_matching) {
        coro_channel_select_type_matching_test();
    }
};
#else
int main(int, char* []) {
    return coro_channel_select_type_matching_test();
}
#endif
