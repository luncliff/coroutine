//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include "test.h"

using namespace coro;

constexpr int bye = 0;

auto consumer(channel<int>& ch) -> forget_frame {
    // the type doesn't support for-co_await for now
    for (auto [msg, ok] = co_await ch.read(); ok;
         tie(msg, ok) = co_await ch.read()) {
        // ok == true: we received a value
        if (msg == bye) {
            _println_("consumer loop exit");
            break;
        }
    }

    // we know that we received the `bye`,
    // but will read again to check `ok` becomes false
    auto [_, ok] = co_await ch.read();
    // ok == false: channel is going to be destroyed (no effect for read/write)
    _require_(ok == false, __FILE__, __LINE__);
    _println_("channel destruction detected");
}

auto producer_owner() -> forget_frame {
    channel<int> ch{};
    consumer(ch); // start a consumer routine

    for (int msg : {1, 2, 3, bye}) {
        auto ok = co_await ch.write(msg);
        // ok == true: we sent a value
        _require_(ok == true, __FILE__, __LINE__);
    }
    _println_("producer loop exit");
    _println_("producer_owner is going to return");
}

auto coro_channel_ownership_producer_test() {
    producer_owner();
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_channel_ownership_producer_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_channel_ownership_producer
    : public TestClass<coro_channel_ownership_producer> {
    TEST_METHOD(test_coro_channel_ownership_producer) {
        coro_channel_ownership_producer_test();
    }
};
#endif
