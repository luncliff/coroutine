// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

thread_local thread_data current_data{};

thread_id_t current_thread_id() noexcept
{
    // trigger thread local object instantiation
    return current_data.get_id();
}

#ifdef _MSC_VER

#include <Windows.h> // System API

extern thread_registry registry;

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
