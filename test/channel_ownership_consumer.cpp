//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using namespace concrt;

constexpr int bye = 0;

auto producer(channel<int>& ch) -> no_return {

    for (int msg : {1, 2, 3, bye}) {
        auto ok = co_await ch.write(msg);
        // ok == true: we sent a value
        REQUIRE(ok);
    }
    PRINT_MESSAGE("producer loop exit"s);

    // we know that we sent the `bye`,
    // but will send again to check `ok` becomes `false`
    int msg = -1;
    auto ok = co_await ch.write(msg);
    // ok == false: channel is going to be destroyed (no effect for read/write)
    REQUIRE(ok == false);
    PRINT_MESSAGE("channel destruction detected"s);
}

auto consumer_owner() -> no_return {
    channel<int> ch{};
    producer(ch); // start a producer routine

    // the type doesn't support for-co_await for now
    for (auto [msg, ok] = co_await ch.read(); ok;
         tie(msg, ok) = co_await ch.read()) {
        // ok == true: we sent a value
        if (msg == bye) {
            PRINT_MESSAGE("consumer loop exit"s);
            break;
        }
    }
    PRINT_MESSAGE("consumer_owner is going to return"s);
}

auto coro_channel_ownership_consumer_test() {
    consumer_owner();
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_channel_ownership_consumer_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_channel_ownership_consumer
    : public TestClass<coro_channel_ownership_consumer> {
    TEST_METHOD(test_coro_channel_ownership_consumer) {
        coro_channel_ownership_consumer_test();
    }
};
#endif
