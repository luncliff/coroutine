//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>
#include <coroutine/thread.h>

#include <concurrency_helper.h>

#include "test.h"
using namespace std;
using namespace coro;

using channel_section_t = channel<uint64_t, section>;

auto coro_channel_no_leak_under_race_test() {
    static constexpr size_t max_try_count = 6;

    uint32_t success{}, failure{};
    latch group{2 * max_try_count};

    auto send_with_callback = [&](channel_section_t& ch,
                                  uint64_t value) -> forget_frame {
        co_await ptp_work{};

        auto w = co_await ch.write(value);
        w ? success += 1 : failure += 1;
        group.count_down();
    };
    auto recv_with_callback = [&](channel_section_t& ch) -> forget_frame {
        co_await ptp_work{};

        auto [value, r] = co_await ch.read();
        r ? success += 1 : failure += 1;
        group.count_down();
    };

    channel_section_t ch{};

    // Spawn coroutines
    uint64_t repeat = max_try_count;
    while (repeat--) {
        recv_with_callback(ch);
        send_with_callback(ch, repeat);
    }

    // Wait for all coroutines...
    // !!! user should ensure there is no race for destroying channel !!!
    group.wait();

    // for same read/write operation,
    //  channel guarantees all reader/writer will be executed.
    _require_(failure == 0, __FILE__, __LINE__);

    // however, the mutex in the channel is for matching of the coroutines.
    // so the counter in the context will be raced
    _require_(success <= 2 * max_try_count);
    _require_(success > 0);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_channel_no_leak_under_race_test();
}
#elif __has_include(<CppUnitTest.h>)
class coro_channel_no_leak_under_race
    : public TestClass<coro_channel_no_leak_under_race> {
    TEST_METHOD(test_coro_channel_no_leak_under_race) {
        coro_channel_no_leak_under_race_test();
    }
};
#endif
