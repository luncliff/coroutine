// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/suspend.h>

#include <atomic>
#include <gsl/gsl>

#include "suspend/message_queue.h"

struct suspend_queue_impl final
{
    std::atomic<uint32_t> ref_count{};
    uint32_t pad1{};
    std::unique_ptr<messaging_queue_t> mq{};
};

GSL_SUPPRESS(type.1)
auto get_impl(gsl::not_null<suspend_queue*> p) noexcept
    -> gsl::not_null<suspend_queue_impl*>
{
    static_assert(sizeof(suspend_queue) >= sizeof(suspend_queue_impl));
    return reinterpret_cast<suspend_queue_impl*>(p.get());
}

suspend_queue::suspend_queue() noexcept(false) : storage{}
{
    auto* self = new (get_impl(this)) suspend_queue_impl{};
    self->mq = create_message_queue();
}

suspend_queue::~suspend_queue() noexcept
{
    get_impl(this)->~suspend_queue_impl();
}

void suspend_queue::push(coroutine_task_t coro) noexcept(false)
{
    size_t retry_count = 5000;
    message_t m{};

    m.ptr = coro.address();
    while (get_impl(this)->mq->post(m) == false)
        if (--retry_count)
            continue;
        else
            throw std::runtime_error{"can't push to suspend queue"};
}

bool suspend_queue::try_pop(coroutine_task_t& coro) noexcept
{
    message_t m{};
    if (get_impl(this)->mq->peek(m))
    {
        coro = coroutine_task_t::from_address(m.ptr);
        return true;
    }
    return false;
}
