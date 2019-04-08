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

#include "../suspend/circular_queue.hpp"

namespace cc
{
using namespace std;
using concurrency::reader_writer_lock;

class concrt_circular_queue final : public limited_lock_queue
{
    reader_writer_lock rwlock{};
    bounded_circular_queue_t<value_type> cq{};

  public:
    queue_op_status wait_push(value_type v) override
    {
        unique_lock lck{rwlock};
        if (cq.push(v) == false)
            return queue_op_status::full;
        return queue_op_status::success;
    }
    queue_op_status wait_pop(value_type& ref) override
    {
        unique_lock lck{rwlock};
        if (cq.try_pop(ref) == false)
            return queue_op_status::empty;
        return queue_op_status::success;
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
    void open() noexcept override
    {
        // no close operation for this type
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

auto make_lock_queue(int v) -> std::unique_ptr<limited_lock_queue>
{
    return std::make_unique<concrt_circular_queue>();
}

} // namespace cc

// adapter for version 1.4.x

using namespace std;
using namespace std::experimental;
using namespace cc;

struct sq_adapter final
{
    unique_ptr<limited_lock_queue> mq{};
};

GSL_SUPPRESS(type .1)
auto cast(gsl::not_null<suspend_queue*> p) noexcept
    -> gsl::not_null<sq_adapter*>
{
    static_assert(sizeof(suspend_queue) >= sizeof(sq_adapter));
    return reinterpret_cast<sq_adapter*>(p.get());
}

suspend_queue::suspend_queue() noexcept(false) : storage{}
{
    cast(this)->mq = cc::make_lock_queue();
}
suspend_queue::~suspend_queue() noexcept
{
    auto trunc = move(cast(this)->mq);
}

void suspend_queue::push(coroutine_task_t coro) noexcept(false)
{
    cast(this)->mq->wait_push(coro.address());
}
bool suspend_queue::try_pop(coroutine_task_t& coro) noexcept
{
    try
    {
        void* ptr = nullptr;
        if (cast(this)->mq->wait_pop(ptr) == queue_op_status::success)
            coro = coroutine_handle<void>::from_address(ptr);

        return coro;
    }
    catch (...)
    {
        return false;
    }
}
