// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <system_error>

#include "posix/adapter.h"

static_assert(sizeof(pthread_rwlock_t) <= sizeof(section));

auto* for_posix(section* s) noexcept
{
    return reinterpret_cast<pthread_rwlock_t*>(s);
}
auto* for_posix(const section* s) noexcept
{
    return reinterpret_cast<const pthread_rwlock_t*>(s);
}

section::section(uint16_t) noexcept(false) : u64{}
{
    u64[0] = 0;
    auto* rwlock = for_posix(this);

    if (auto ec = pthread_rwlock_init(rwlock, nullptr))
        throw std::system_error{
            ec, std::system_category(), "pthread_rwlock_init"};
}

section::~section() noexcept
{
    auto* rwlock = for_posix(this);

    if (auto ec = pthread_rwlock_destroy(rwlock))
        std::fputs(
            std::system_error{ec, std::system_category(), "pthread_rwlock_init"}
                .what(),
            stderr);
}

bool section::try_lock() noexcept
{
    auto* rwlock = for_posix(this);
    // EBUSY  // possible error
    // EINVAL
    // EDEADLK
    auto ec = pthread_rwlock_trywrlock(rwlock);
    return ec == 0;
}

// - Note
//
//  There was an issue with `pthread_mutex_`
//  it returned EINVAL for lock operation
//  replacing it the rwlock
//
void section::lock() noexcept(false)
{
    auto* rwlock = for_posix(this);

    if (auto ec = pthread_rwlock_wrlock(rwlock))
        // EINVAL ?
        throw std::system_error{
            ec, std::system_category(), "pthread_rwlock_wrlock"};
}

void section::unlock() noexcept(false)
{
    auto* rwlock = for_posix(this);

    if (auto ec = pthread_rwlock_unlock(rwlock))
        throw std::system_error{
            ec, std::system_category(), "pthread_rwlock_unlock"};
}
