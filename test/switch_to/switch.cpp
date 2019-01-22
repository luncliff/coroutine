
#include "switch.h"

using coroutine_task_t = std::experimental::coroutine_handle<void>;

auto switch_to_2::schedule() noexcept -> enumerable<coroutine_task_t>
{
    std::unique_lock lck{mtx};

    while (queue.empty() == false)
    {
        auto task = queue.front();
        queue.pop_front();
        lck.unlock();

        co_yield task;
        lck.lock();
    }
}

bool switch_to_2::await_ready() const noexcept
{
    return false; // always trigger scheduling
}

void switch_to_2::await_suspend(coroutine_task_t coro) noexcept(false)
{
    std::unique_lock lck{mtx};
    queue.push_back(coro);
}

void switch_to_2::await_resume() noexcept
{
}
