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
//  To Do
//      Save CPU power
//      Timed wait for peek_switched
//      Remain idle background is not used (possibly with signal)
//
// ---------------------------------------------------------------------------
#include <coroutine/frame.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>
#include <thread/types.h>

#include <cassert>
#include <csignal>
#include <future>
#include <string>
#include <system_error>

#include <pthread.h>   // implement over pthread
#include <sched.h>     // for scheduling customization
#include <sys/types.h> // system types

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

using namespace std;
using namespace std::experimental;
using namespace std::literals;

//
// !!! notice that this is not atomic !!!
//
// Worker id here can be modified dynamically.
// The library must ensure it's always holding a valid id.
// If the requirement is fulfilled, it is ok not to use atomic for here
//
thread_id_t background_thread_id;

void* resume_coroutines_on_backgound(thread_registry&) noexcept(false);

// - Note
//      attach only 1 worker for now. unless it's necessary,
//      thread pool approach won't be used to minimize code of this library
LIB_PROLOGUE void setup_worker() noexcept(false)
{
    static_assert(sizeof(pthread_t) == sizeof(void*));
    try
    {
        auto fthread = reinterpret_cast<void* (*)(void*)>(
            resume_coroutines_on_backgound);

        if (auto ec = pthread_create(
                reinterpret_cast<pthread_t*>(addressof(background_thread_id)),
                nullptr, fthread, get_thread_registry()))
            // expect successful worker creation. unless kill the program
            throw system_error{ec, system_category(), "pthread_create"};

        // it rarely happens that background_thread_id is not modified
        assert(background_thread_id != thread_id_t{});
    }
    catch (const std::system_error& e)
    {
        perror(e.what());
    }
}

LIB_EPILOGUE void teardown_worker() noexcept(false)
{
    const pthread_t worker = (pthread_t)background_thread_id;

    pthread_cancel(worker);
    if (auto ec = pthread_join(worker, nullptr))
        throw system_error{ec, system_category(), "pthread_join"};
}

void* resume_coroutines_on_backgound(thread_registry& reg) noexcept(false)
{
    coroutine_handle<void> coro{};

    const auto tid = current_thread_id();
    // register this thread to receive messages
    *(reg.find_or_insert(tid)) = get_local_data();

    // the variable is set by `pthread_create`,
    //  but make it sure since we are in another thread
    background_thread_id = static_cast<thread_id_t>(tid);

ResumeNext:
    try
    {
        while (peek_switched(coro) == true)
        {
            coro.resume();
            coro = nullptr;
        }

        pthread_testcancel();
        // std::this_thread::sleep_for(1ms);
    }
    catch (const error_code& e)
    {
        ::fputs(e.message().c_str(), stderr);
    }
    catch (const error_condition& e)
    {
        ::fputs(e.message().c_str(), stderr);
    }
    catch (const exception& e)
    {
        ::fputs(e.what(), stderr);
    }
    goto ResumeNext;

    return nullptr;
}

void post_to_background(
    std::experimental::coroutine_handle<void> coro) noexcept(false)
{
    message_t msg{};
    msg.ptr = coro.address();

    // if post failes,
    //  give some time consume messages
    while (post_message(background_thread_id, msg) == false)
        std::this_thread::yield();
}
