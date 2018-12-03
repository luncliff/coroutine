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
#include "./adapter.h"
#include <coroutine/frame.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

#include <cassert>
#include <csignal>
#include <future>
#include <system_error>

#define LIB_PROLOGUE __attribute__((constructor))

//
// !!! notice that this is not atomic !!!
//
// Worker id here can be modified dynamically.
// The library must ensure it's always holding a valid id.
// If the requirement is fulfilled, it is ok not to use atomic for here
//
thread_id_t unknown_worker_id;

// additional namespace to prevent name collision
namespace itr2 // internal 2
{
// - Note
//      Simple worker group to manage available threads
class worker_group_t
{
    // any shared pthread attributes ?

  public:
    worker_group_t() noexcept = default;
    ~worker_group_t() noexcept = default;

  public:
    void add(const pthread_t tid) noexcept(false);
    void remove(const pthread_t tid) noexcept;
};
worker_group_t unknown{};

void process_with_group(worker_group_t& workers) noexcept(false);

// - Note
//      attach only 1 worker for now. unless it's necessary,
//      thread pool approach won't be used to minimize code of this library
LIB_PROLOGUE void start_worker_group() noexcept(false)
{
    static_assert(sizeof(pthread_t) == sizeof(thread_id_t));

    void* (*fthread)(void*) = nullptr;
    fthread = reinterpret_cast<decltype(fthread)>(process_with_group);

    auto ec = pthread_create(
        reinterpret_cast<pthread_t*>(std::addressof(unknown_worker_id)),
        nullptr, fthread, std::addressof(unknown));

    if (ec)
    {
        // print error for possible error stream redirection
        ::perror(strerror(ec));
        // expect successful worker creation. unless kill the program
        throw std::system_error{ec, std::system_category(), "pthread_create"};
    }

    // std::fprintf(stdout, //
    //              "created background thread %lx \n",
    //              unknown_worker_id);
}

void remove_worker(const pthread_t& tid) noexcept(false)
{
    unknown.remove(tid);
}

void process_with_group(worker_group_t& workers) noexcept(false)
{
    const pthread_t tid = pthread_self();

    std::experimental::coroutine_handle<void> coro{};

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

    workers.add(tid);

    pthread_cleanup_push(
        reinterpret_cast<void (*)(void*)>(remove_worker),     //
        reinterpret_cast<void*>(const_cast<pthread_t*>(&tid)) //
    );

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);

// using goto to reduce indentation
TryProcessing:
    pthread_testcancel(); // cancel by manager?

    try
    {
        if (peek_switched(coro)) // peek and continue the works
        {
            // std::printf("processing ... %p \n", coro.address());
            coro.resume();
        }

        using namespace std::literals;
        std::this_thread::sleep_for(2s);
    }
    catch (const std::exception& e)
    {
        // need to acquire more detailed information here
        ::perror(e.what());

        // for exception, forward the exception to handler and keep going
        goto OnCriticalError;
    }
    catch (...)
    {
        ::perror("thread caught unknown exception");

        // The error was so critical
        //  that there is no way but to end this thread
        // auto ex = std::current_exception();

        // Remind again:
        // !!! This is CRITICAL so it can't be ignored !!!
        // std::rethrow_exception(ex);
        goto OnCriticalError;
    }
    goto TryProcessing;

OnCriticalError:
    pthread_cleanup_pop(true);
}

void worker_group_t::add(const pthread_t tid) noexcept(false)
{
    static_assert(sizeof(pthread_t) == sizeof(thread_id_t));

    // std::printf("worker_group_t::add %lu\n", tid);
    // !!! export some worker thread !!!
    unknown_worker_id
        = static_cast<thread_id_t>(reinterpret_cast<uint64_t>(tid));
}

void worker_group_t::remove(const pthread_t tid) noexcept
{
    // std::printf("worker_group_t::remove %lu\n", tid);

    // !!! export some worker thread !!!
    unknown_worker_id = thread_id_t{};
}

} // namespace itr2
