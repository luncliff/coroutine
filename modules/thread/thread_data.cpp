// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

thread_id_t current_thread_id() noexcept
{
    // trigger thread local object instantiation
    return get_local_data()->get_id();
}

#if __APPLE__ || __linux__ || __unix__

#include <csignal>
#include <pthread.h>

bool check_thread_exists(thread_id_t id) noexcept
{
    // expect ESRCH for return error code
    if (pthread_kill((pthread_t)id, 0) == 0)
        return true;

    return false;
}

thread_data::thread_data() noexcept(false) : cv{}, queue{}
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);

    auto ptr = get_thread_registry() //
                   ->find_or_insert( //
                       static_cast<thread_id_t>(tid));
    *ptr = this;
}

thread_data::~thread_data() noexcept
{
    get_thread_registry()->erase( //
        this->get_id());          // this line might throw exception.
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

thread_data::thread_data() noexcept(false) : cv{}, queue{}
{
    const auto tid = static_cast<thread_id_t>(GetCurrentThreadId());
    auto ptr = get_thread_registry()->find_or_insert(tid);
    *ptr = this;
}

thread_data::~thread_data() noexcept
{
    // this line might throw exception.
    // which WILL kill the program.
    get_thread_registry()->erase(get_id());
}

thread_id_t thread_data::get_id() const noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

#endif
