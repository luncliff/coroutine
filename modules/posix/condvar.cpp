// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./adapter.h"

posix_condvar_t::posix_condvar_t() noexcept(false)
{
    // init attr?
    pthread_mutex_init(&mtx, nullptr);
    pthread_cond_init(&cv, nullptr);
}

posix_condvar_t::~posix_condvar_t() noexcept
{
    // destry attr?
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cv);
}

void posix_condvar_t::wait(uint32_t ms) noexcept
{
    timespec timeout{};
    timeout.tv_nsec = ms * 1'000'000; // ms -> nano

    pthread_mutex_lock(&mtx);
    pthread_cond_timedwait(&cv, &mtx, &timeout);
    pthread_mutex_unlock(&mtx);
}
void posix_condvar_t::signal() noexcept
{
    // expect some unknown error. ignore
    pthread_cond_signal(&cv);
}
