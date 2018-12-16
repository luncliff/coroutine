// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

extern thread_registry registry;

#if __APPLE__ || __linux__ || __unix__

#include <csignal>
#include <pthread.h>

thread_local thread_data current_data{};

thread_id_t current_thread_id() noexcept
{
    // trigger thread local object instantiation
    return current_data.get_id();
}

bool check_thread_exists(thread_id_t id) noexcept
{
    // expect ESRCH for return error code
    if (pthread_kill((pthread_t)id, 0) == 0)
        return true;

    return false;
}

thread_data::thread_data() noexcept(false) : queue{}
{
    // ... thread life start. trigger setup ...
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);

    auto ptr = registry.reserve(tid);
    *ptr = this;
}

thread_data::~thread_data() noexcept
{
    // ... thread life end. trigger teardown ...
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);

    registry.remove(tid); // this line might throw exception.
                          // which WILL kill the program.
}

thread_id_t thread_data::get_id() const noexcept
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);
    return static_cast<thread_id_t>(tid);
}

#elif _MSC_VER

#include <Windows.h> // System API

thread_id_t current_thread_id() noexcept
{
    // return static_cast<thread_id_t>(GetCurrentThreadId());
    return get_local_data()->get_id();
}

thread_data::thread_data() noexcept(false) : queue{}
{
    // ... thread life start. trigger setup ...
    const auto tid = static_cast<uint64_t>(GetCurrentThreadId());
    auto ptr = registry.reserve(tid);
    *ptr = this;
}

thread_data::~thread_data() noexcept
{
    // ... thread life end. trigger teardown ...
    const auto tid = static_cast<uint64_t>(GetCurrentThreadId());
    registry.remove(tid); // this line might throw exception.
                          // which WILL kill the program.
}

thread_id_t thread_data::get_id() const noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

#endif
