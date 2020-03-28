/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */

#undef NDEBUG
#include <cassert>
#include <mutex>

#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include <coroutine/windows.h>
#include <latch.h>

using namespace std;
using namespace coro;

using channel_section_t = channel<uint64_t, mutex>;
#if defined(__GNUC__)
using no_return_t = coro::null_frame_t;
#else
using no_return_t = std::nullptr_t;
#endif

int main(int, char*[]) {
    static constexpr size_t max_try_count = 6;

    uint32_t success{}, failure{};
    latch group{2 * max_try_count};

    auto send_once = [&](channel_section_t& ch, uint64_t value) -> no_return_t {
        co_await continue_on_thread_pool{};

        auto w = co_await ch.write(value);
        w ? success += 1 : failure += 1;
        group.count_down();
    };
    auto recv_once = [&](channel_section_t& ch) -> no_return_t {
        co_await continue_on_thread_pool{};

        auto [value, r] = co_await ch.read();
        r ? success += 1 : failure += 1;
        group.count_down();
    };

    channel_section_t ch{};

    // Spawn coroutines
    uint64_t repeat = max_try_count;
    while (repeat--) {
        recv_once(ch);
        send_once(ch, repeat);
    }

    // Wait for all coroutines...
    // !!! user should ensure there is no race for destroying channel !!!
    group.wait();
    // for same read/write operation,
    //  channel guarantees all reader/writer will be executed.
    assert(failure == 0);

    // however, the mutex in the channel is for matching of the coroutines.
    // so the counter in the context will be raced
    assert(success <= 2 * max_try_count);
    assert(success > 0);
    return EXIT_SUCCESS;
}
