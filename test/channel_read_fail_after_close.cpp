//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include "test.h"
using namespace std;
using namespace coro;

using channel_without_lock_t = channel<int>;

auto coro_channel_read_return_false_after_close_test() {
    auto ch = make_unique<channel_without_lock_t>();
    bool ok = true;

    auto coro_read = [&ok](auto& ch, auto& value) -> preserve_frame {
        tie(value, ok) = co_await ch.read();
    };

    auto item = int{};
    // coroutine will suspend and wait in the channel
    auto h = coro_read(*ch, item);
    {
        auto truncator = move(ch); // if channel is destroyed ...
    }
    _require_(ch.get() == nullptr);

    coroutine_handle<void>& coro = h;
    _require_(coro.done()); // coroutine is in done state
    coro.destroy();         // destroy to prevent leak

    _require_(ok == false); // and channel returned false
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_channel_read_return_false_after_close_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_channel_read_return_false_after_close
    : public TestClass<coro_channel_read_return_false_after_close> {
    TEST_METHOD(test_coro_channel_read_return_false_after_close) {
        coro_channel_read_return_false_after_close_test();
    }
};
#endif
