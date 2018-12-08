// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

extern thread_registry registry;
thread_local thread_data current_data{};

thread_id_t current_thread_id() noexcept
{
    // trigger thread local object instantiation
    return current_data.get_id();
}

#if __APPLE__ || __linux__ || __unix__

#include <pthread.h>

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

thread_data::thread_data() noexcept(false) : queue{}
{
    try
    {
        // ... thread life start. trigger setup ...
        const auto tid = static_cast<uint64_t>(GetCurrentThreadId());
        auto ptr = registry.reserve(tid);
        *ptr = this;
    }
    catch (const std::exception& ex)
    {
        ::perror(ex.what());
        throw ex;
    }
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
