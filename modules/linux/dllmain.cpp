// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <csignal>
#include <pthread.h>   // implement over pthread API
#include <sched.h>     // for scheduling customization
#include <sys/types.h> // system types

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

thread_id_t current_thread_id() noexcept
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);
    return static_cast<thread_id_t>(tid);
}

bool check_thread_exists(thread_id_t id) noexcept
{
    if (id == thread_id_t{})
        return false;

    // expect ESRCH for return error code
    return pthread_kill((pthread_t)id, 0) == 0;
}
