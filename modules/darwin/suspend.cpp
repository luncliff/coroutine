// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>
#include <coroutine/suspend.h>

#include <gsl/gsl>
#include <mutex>

#include <sys/types.h>

#include "circular_queue.hpp"

using namespace std;

namespace coro
{

// for pthread_mutex_t,  don't forget to use
//  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
//  pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);

class concrt_circular_queue final : public limited_lock_queue
{
    concrt::pthread_section mtx{};
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
