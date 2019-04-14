//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once

#include <coroutine/channel.hpp>
#include <coroutine/return.h>

#include <array>
#include <thread>

// lockable without lock operation
struct bypass_lock
{
    bool try_lock() noexcept
    {
        return true;
    }
    void lock() noexcept
    {
        // do nothing since this is bypass lock
    }
    void unlock() noexcept
    {
        // it is not locked
    }
};

void test_require_true(bool cond);

using namespace std;
using namespace std::experimental;
using namespace coro;

// ensure successful write to channel
template <typename E, typename L>
auto write_to(channel<E, L>& ch, E value, bool ok = false) -> return_ignore
{
    using namespace std;

    ok = co_await ch.write(value);
    if (ok == false)
        // !!!!!
        // seems like optimizer is removing `value`.
        // so using it in some pass makes
        // the symbol and its memory location alive
        // !!!!!
        value += 1;

    test_require_true(ok);
}

// ensure successful read from channel
template <typename E, typename L>
auto read_from(channel<E, L>& ch, E& value, bool ok = false) -> return_ignore
{
    using namespace std;

    tie(value, ok) = co_await ch.read();
    test_require_true(ok);
}
