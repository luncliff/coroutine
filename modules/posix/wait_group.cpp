// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <system_error>

#include "./adapter.h"

struct wait_group_posix final
{
    posix_condvar_t event{};
    std::atomic<uint32_t> count{};
};

auto for_posix(wait_group* wg) noexcept
{
    static_assert(sizeof(wait_group_posix) <= sizeof(wait_group));
    return reinterpret_cast<wait_group_posix*>(wg);
}

wait_group::wait_group() noexcept(false) : storage{}
{
    new (for_posix(this)) wait_group_posix{};
}

wait_group::~wait_group() noexcept
{
    auto* wg = for_posix(this);

    // possible logic error in the case. but just delete
    wg->event.~posix_condvar_t();
}

void wait_group::add(uint16_t delta) noexcept
{
    auto* wg = for_posix(this);

    // just increase count.
    // notice that delta is always positive
    wg->count.fetch_add(delta);
}

void wait_group::done() noexcept
{
    auto* wg = for_posix(this);

    if (wg->count > 0)
        wg->count.fetch_sub(1);

    if (wg->count.load() != 0)
        return;

    // notify
    wg->event.signal();
}

bool wait_group::wait(duration timeout) noexcept(false)
{
    using namespace std::chrono;
    auto* wg = for_posix(this);

    // start timer

    wg->event.wait(duration_cast<milliseconds>(timeout).count());

    // pick timer. if elapsed time is longer, consider failed
    return false;
}
