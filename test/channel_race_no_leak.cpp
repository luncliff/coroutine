//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

using channel_section_t = channel<uint64_t, section>;

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
// work like `ptp_work` of win32 based interface, but rely on standard
class ptp_work final : no_copy_move {
    coroutine_handle<void> coro;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(coroutine_handle<void> handle) noexcept(false) {
        coro = handle;
        async(launch::async, [this]() { coro.resume(); });
    }
    void await_resume() noexcept {
        coro = nullptr; // forget it
    }
};
#endif

auto coro_channel_no_leak_under_race_test() {
    static constexpr size_t max_try_count = 6;

    uint32_t success{}, failure{};
    latch group{2 * max_try_count};

    auto send_with_callback = [&](channel_section_t& ch,
                                  uint64_t value) -> no_return {
        co_await ptp_work{};

        auto w = co_await ch.write(value);
        w ? success += 1 : failure += 1;
        group.count_down();
    };
    auto recv_with_callback = [&](channel_section_t& ch) -> no_return {
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
    REQUIRE(failure == 0);

    // however, the mutex in the channel is for matching of the coroutines.
    // so the counter in the context will be raced
    REQUIRE(success <= 2 * max_try_count);
    REQUIRE(success > 0);

    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class coro_channel_no_leak_under_race
    : public TestClass<coro_channel_no_leak_under_race> {
    TEST_METHOD(test_coro_channel_no_leak_under_race) {
        coro_channel_no_leak_under_race_test();
    }
};
#else
int main(int, char* []) {
    return coro_channel_no_leak_under_race_test();
}
#endif
