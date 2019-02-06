
#include "switch.h"

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
    std::lock_guard lck{mtx};
    queue.push_back(coro);
}

void switch_to_2::await_resume() noexcept
{
}
