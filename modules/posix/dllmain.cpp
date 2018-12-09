// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <thread/types.h>

#include <cassert>
#include <numeric>

#include <pthread.h>   // implement over pthread API
#include <sched.h>     // for scheduling customization
#include <sys/types.h> // system types

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

extern thread_local thread_data current_data;

thread_data* get_local_data() noexcept
{
    return std::addressof(current_data);
}

void lazy_delivery(thread_id_t thread_id, message_t msg) noexcept(false)
{
    throw std::runtime_error{"lazy_delivery is not implemented"};
}
