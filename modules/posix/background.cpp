// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Note
//      Code for worker thread management.
//      Unless it's necessary, thread pool approach won't be used to minimize
//      code of this library.
//
//      So the code below focus on maintaining only 1 thread
//      in safe, robust manner
//
// ---------------------------------------------------------------------------
#include <coroutine/frame.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <thread/types.h>

#include <cassert>
#include <csignal>
#include <future>
#include <system_error>

#include <pthread.h>   // implement over pthread
#include <sched.h>     // for scheduling customization
#include <sys/types.h> // system types

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

//
// !!! notice that this is not atomic !!!
//
// Worker id here can be modified dynamically.
// The library must ensure it's always holding a valid id.
// If the requirement is fulfilled, it is ok not to use atomic for here
//
thread_id_t background_thread_id;

extern thread_registry registry;
extern thread_local thread_data current_data;
void* resume_coroutines_on_backgound(thread_registry& registry) noexcept(false);

// - Note
//      attach only 1 worker for now. unless it's necessary,
//      thread pool approach won't be used to minimize code of this library
LIB_PROLOGUE void setup_worker() noexcept(false)
{
    static_assert(sizeof(pthread_t) == sizeof(thread_id_t));

    void* (*fthread)(void*) = nullptr;
    fthread
        = reinterpret_cast<decltype(fthread)>(resume_coroutines_on_backgound);

    if (auto ec = pthread_create(
            reinterpret_cast<pthread_t*>(std::addressof(background_thread_id)),
            nullptr, fthread, std::addressof(registry)))
        // expect successful worker creation. unless kill the program
        throw std::system_error{ec, std::system_category(), "pthread_create"};

    assert(background_thread_id != thread_id_t{});
}

LIB_EPILOGUE void teardown_worker() noexcept(false)
{
    const pthread_t worker = (pthread_t)background_thread_id;

    pthread_cancel(worker);
    if (auto ec = pthread_join(worker, nullptr))
        throw std::system_error{ec, std::system_category(), "pthread_join"};
}

using namespace std::experimental;

void* resume_coroutines_on_backgound(thread_registry& reg) noexcept(false)
{
    coroutine_handle<void> coro{};

    // the variable is set by `pthread_create`,
    //  but make it sure since we are in another thread
    background_thread_id = current_thread_id();

    const auto tid = static_cast<uint64_t>(background_thread_id);
    // register this thread to receive messages
    *(registry.reserve(tid)) = std::addressof(current_data);

    try
    {
    ResumeNext:
        if (peek_switched(coro) == true)
        {
            coro.resume();
            coro = nullptr;
        }

        pthread_testcancel();
        goto ResumeNext;
    }
    catch (const std::exception& e)
    {
        ::fputs(e.what(), stderr);
    }
    catch (...)
    {
        ::fputs(__FUNCTION__, stderr);
    }
    return nullptr;
}