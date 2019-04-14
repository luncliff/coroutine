// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/suspend.h>

#include <concrt.h> // Windows Concurrency Runtime
#include <gsl/gsl>
#include <mutex>

#include "circular_queue.hpp"

namespace coro
{
using namespace std;
using concurrency::reader_writer_lock;

class concrt_circular_queue final : public limited_lock_queue
{
    reader_writer_lock rwlock{};
    bounded_circular_queue_t<value_type> cq{};

  public:
    bool wait_push(value_type v) override
    {
        unique_lock lck{rwlock};
        return cq.push(v) == false;
    }
    bool wait_pop(value_type& ref) override
    {
        unique_lock lck{rwlock};
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
    return std::make_unique<concrt_circular_queue>();
}

} // namespace coro
