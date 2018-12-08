// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

using namespace std::literals;
using namespace std::experimental;

class SwitchingTest : public TestClass<SwitchingTest>
{
    static auto SwitchOnce(wait_group& wg, uint64_t& before,
                           uint64_t& after) noexcept(false) -> unplug
    {
        switch_to back{}; // 0 means background thread

        before = GetCurrentThreadId();
        co_await back; // switch to designated thread

        after = GetCurrentThreadId();
        wg.done();
    }

    TEST_METHOD(SwitchToUnknown)
    {
        wait_group wg{};
        uint64_t id1, id2;

        wg.add(1);
        SwitchOnce(wg, id1, id2);
        Assert::IsTrue(wg.wait());

        // get different thread id
        Assert::IsTrue(id1 != id2);
    }

    static auto SwitchToSpeicified( // switch twice...
        wait_group& wg, uint64_t target) noexcept(false) -> unplug
    {
        const auto start = GetCurrentThreadId();
        switch_to back{}, fore{target}; // 0 means background thread

        co_await back;
        Assert::IsTrue(start != GetCurrentThreadId());

        co_await fore;
        Assert::IsTrue(target == GetCurrentThreadId());

        wg.done();
    }

    TEST_METHOD(SwitchToMain)
    {
        coroutine_handle<void> coro{};
        wait_group wg{};
        uint64_t id = GetCurrentThreadId();

        wg.add(1);
        SwitchToSpeicified(wg, id);

        while (peek_switched(coro) == false)
            // retry until we fetch the coroutine
            continue;

        coro.resume();

        Assert::IsTrue(wg.wait());
    }
};
