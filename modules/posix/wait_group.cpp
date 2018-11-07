// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <system_error>

#include "./adapter.h"

wait_group::wait_group() noexcept(false) //
    : event{new posix_cond_var{}}, count{0}
{
}

wait_group::~wait_group() noexcept
{
    if (event == nullptr) return;
    // possible logic error in the case. but just delete
    auto* cv = reinterpret_cast<posix_cond_var*>(event);
    event = nullptr;
    delete cv;
}

void wait_group::add(uint32_t delta) noexcept
{
    // just increase count.
    // notice that delta is always positive
    count.fetch_add(delta);
}

void wait_group::done() noexcept
{
    if (event == nullptr) // already done. nothing to do
        return;

    if (count > 0) count.fetch_sub(1);

    if (count.load() != 0) return;

    // notify
    auto* cv = reinterpret_cast<posix_cond_var*>(event);
    cv->signal();
}

void wait_group::wait(uint32_t timeout) noexcept(false)
{
    auto* cv = reinterpret_cast<posix_cond_var*>(event);
    cv->wait(timeout);
    event = nullptr;
    delete cv;
}
