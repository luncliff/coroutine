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
#if defined(__GNUC__)
using no_return_t = coro::null_frame_t;
#else
using no_return_t = std::nullptr_t;
#endif

int main(int, char*[]) {
    u32_chan_t ch1{};
    i32_chan_t ch2{};

    auto write_to = [](auto& ch, auto value, bool ok = false) -> no_return_t {
        ok = co_await ch.write(value);
        if (ok == false)
            // !!!!!
            // seems like clang optimizer is removing `value`.
            // so using it in some pass makes
            // the symbol and its memory location alive
            // !!!!!
            value += 1;
        assert(ok);
    };

    write_to(ch1, 17u);

    // the callback functions can be a coroutine (which includes
    // subroutine), but their returns will be truncated !
    select(
        ch2,
        [](auto v) {
            static_assert(is_same_v<decltype(v), int32_t>);
            fputs("select on empty channel must bypass", stderr);
            exit(__LINE__);
        },
        ch1,
        [](auto v) -> no_return_t {
            static_assert(is_same_v<decltype(v), uint32_t>);
            assert(v == 17u);
            co_await suspend_never{};
        });

    return EXIT_SUCCESS;
}
