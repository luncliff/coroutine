
#pragma once

#include <coroutine/frame.h>
#include <coroutine/return.h>
#include <coroutine/enumerable.hpp>

#include <mutex>
#include <deque>

using coroutine_task_t = std::experimental::coroutine_handle<void>;

class switch_to_2 final
{
    std::deque<coroutine_task_t> queue{};
    std::mutex mtx{};

  public:
    auto schedule() noexcept -> enumerable<coroutine_task_t>;

  public:
    bool await_ready() const noexcept;
    void await_suspend(coroutine_task_t coro) noexcept(false);
    void await_resume() noexcept;
};
