// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <thread/types.h>

#include <cassert>
#include <csignal>
#include <numeric>

#include <pthread.h>   // implement over pthread API
#include <sched.h>     // for scheduling customization
#include <sys/types.h> // system types

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

bool check_thread_exists(thread_id_t id) noexcept
{
    if (id == thread_id_t{})
        return false;

    // expect ESRCH for return error code
    return pthread_kill((pthread_t)id, 0) == 0;
}

void lazy_delivery(thread_id_t thread_id, message_t msg) noexcept(false)
{
    throw std::runtime_error{
        "The library requires user code to invoke `current_thread_id` in "
        "receiver thread before `post_message` to it"};
}
