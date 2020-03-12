/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */

#undef NDEBUG
#include <cassert>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>

using namespace std;
using namespace coro;

using u32_chan_t = channel<uint32_t>;
using i32_chan_t = channel<int32_t>;

int main(int, char*[]) {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    select(
        ch1,
        [](auto v) {
            static_assert(is_same_v<decltype(v), uint32_t>);
            fputs("select on empty channel must bypass", stderr);
            exit(__LINE__);
        },
        ch2,
        [](auto v) {
            static_assert(is_same_v<decltype(v), int32_t>);
            fputs("select on empty channel must bypass", stderr);
            exit(__LINE__);
        });
    return EXIT_SUCCESS;
}
