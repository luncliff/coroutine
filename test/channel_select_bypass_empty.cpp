//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

using u32_chan_t = channel<uint32_t>;
using i32_chan_t = channel<int32_t>;

auto coro_channel_select_on_empty_test() {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    select(ch1,
           [](auto v) {
               static_assert(is_same_v<decltype(v), uint32_t>);
               FAIL_WITH_MESSAGE("select on empty channel must bypass"s);
           },
           ch2,
           [](auto v) {
               static_assert(is_same_v<decltype(v), int32_t>);
               FAIL_WITH_MESSAGE("select on empty channel must bypass"s);
           });

    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class coro_channel_select_on_empty
    : public TestClass<coro_channel_select_on_empty> {
    TEST_METHOD(test_coro_channel_select_on_empty) {
        coro_channel_select_on_empty_test();
    }
};
#else
int main(int, char* []) {
    return coro_channel_select_on_empty_test();
}
#endif
