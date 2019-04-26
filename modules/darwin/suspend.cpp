// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/suspend.h>

#include <gsl/gsl>
#include <mutex>

#include <pthread.h>
#include <sys/types.h>

#include "circular_queue.hpp"

using namespace std;

class pthread_section final
{
    pthread_mutex_t mtx;

  public:
    pthread_section() noexcept(false) : mtx{}
    {
        int ec = 0;
        const char* message = nullptr;
        pthread_mutexattr_t attr{};
        ec = pthread_mutexattr_init(&attr);
        if (ec)
        {
            message = "pthread_mutexattr_init";
            goto OnSystemError;
        }
        ec = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        if (ec)
        {
            message = "pthread_mutexattr_settype";
            goto OnSystemError;
        }
        ec = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);
        if (ec)
        {
            message = "pthread_mutexattr_setprotocol";
            goto OnSystemError;
        }
        ec = pthread_mutex_init(&mtx, &attr);
        if (ec)
        {
            message = "pthread_mutex_init";
            goto OnSystemError;
        }
        ec = pthread_mutexattr_destroy(&attr);
        if (ec)
        {
            message = "pthread_mutexattr_destroy";
            goto OnSystemError;
        }

        return;
    OnSystemError:
        throw system_error{ec, system_category(), message};
    }

    ~pthread_section() noexcept try
    {
        if (auto ec = pthread_mutex_destroy(&mtx))
            throw system_error{ec, system_category(), "pthread_mutex_destroy"};
    }
    catch (const system_error& e)
    {
        ::perror(e.what());
    }
    catch (...)
    {
        ::perror("Unknown exception in pthread_section dtor");
    }

    bool try_lock() noexcept
    {
        // EBUSY  // possible error
        // EINVAL
        return pthread_mutex_trylock(&mtx) == 0;
    }

    // - Note
    //
    //  There was an issue with `pthread_mutex_`
    //  it returned EINVAL for lock operation
    //  replacing it the rwlock
    //
    void lock() noexcept(false)
    {
        if (auto ec = pthread_mutex_lock(&mtx))
            // EINVAL ?
            throw system_error{ec, system_category(), "pthread_mutex_lock"};
    }

    void unlock() noexcept(false)
    {
        if (auto ec = pthread_mutex_unlock(&mtx))
            throw system_error{ec, system_category(), "pthread_mutex_unlock"};
    }
};

namespace coro
{

class concrt_circular_queue final : public limited_lock_queue
{
    pthread_section mtx{};
    bounded_circular_queue_t<value_type> cq{};

  public:
    bool wait_push(value_type v) override
    {
        unique_lock lck{mtx};
        return cq.push(v);
    }
    bool wait_pop(value_type& ref) override
    {
        unique_lock lck{mtx};
        return cq.try_pop(ref);
    }

    void close() noexcept override
    {
        // no close operation for this type
    }
    bool is_closed() const noexcept override
    {
        // no close operation for this type
        return false;
    }

    bool is_empty() const noexcept override
    {
        return cq.empty();
    }
    bool is_full() const noexcept override
    {
        return cq.is_full();
    }
};

auto make_lock_queue() -> std::unique_ptr<limited_lock_queue>
{
    return make_unique<concrt_circular_queue>();
}

} // namespace coro
