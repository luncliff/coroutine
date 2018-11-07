// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <system_error>

#include "./adapter.h"

static_assert(sizeof(posix_mutex_t) <= sizeof(section));

section::section(uint16_t) noexcept
{
    // delegate the work to `posix_mutex_t`
    // placement init
    new (this->u64) posix_mutex_t{};
}

section::~section() noexcept
{
    auto* mtx = reinterpret_cast<posix_mutex_t*>(this);
    mtx->~posix_mutex_t();
}

bool section::try_lock() noexcept
{
    auto* mtx = reinterpret_cast<posix_mutex_t*>(this);
    return mtx->try_lock();
}

void section::lock() noexcept
{
    auto* mtx = reinterpret_cast<posix_mutex_t*>(this);
    return mtx->lock();
}

void section::unlock() noexcept
{
    auto* mtx = reinterpret_cast<posix_mutex_t*>(this);
    return mtx->unlock();
}
