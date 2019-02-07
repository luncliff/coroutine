//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./suspend_test.h"

#if defined(_MSC_VER)
#include <Windows.h>
#include <sdkddkver.h>

thread_id_t get_current_thread_id() noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <pthread.h>

auto get_current_thread_id() noexcept -> thread_id_t
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);
    return static_cast<thread_id_t>(tid);
}
#endif
