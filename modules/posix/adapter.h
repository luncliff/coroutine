// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#ifndef PTHREAD_ADAPTER_H
#define PTHREAD_ADAPTER_H

#include <cstdint>

#include <pthread.h>   // implement over pthread API
#include <sched.h>     // for scheduling customization
#include <sys/types.h> // system types

struct posix_condvar_t final
{
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};
    pthread_condattr_t attr{};

  public:
    posix_condvar_t() noexcept(false);
    ~posix_condvar_t() noexcept;

  public:
    void wait(uint32_t ms) noexcept;
    void signal() noexcept;
};

#endif // PTHREAD_ADAPTER_H