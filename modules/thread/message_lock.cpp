// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

#define NOMINMAX

#include <array>
#include <cassert>
#include <queue>
#include <system_error>

#include <coroutine/sync.h>

#include "../memory.hpp"

using lock_t = std::lock_guard<section>;
using queue_t = std::queue<message_t>;

struct resource_t
{
    thread_id_t owner;
    uint16_t mark;
    uint16_t index;
};

section protect{};

constexpr uint16_t max_thread_count = 300;

index_pool<resource_t, max_thread_count> pool{};
std::array<section, max_thread_count> queue_lockables{};
std::array<queue_t, max_thread_count> queue_list{};

void setup_indices() noexcept
{
    uint16_t i = 0;
    for (auto& res : pool.space)
        res.index = i++;
}

void teardown_indices() noexcept
{
    for (auto& res : pool.space)
        if (static_cast<uint64_t>(res.owner))
            pool.deallocate(std::addressof(res));
}

uint16_t register_thread(thread_id_t thread_id, const lock_t&) noexcept(false)
{
    auto* res = reinterpret_cast<resource_t*>(pool.allocate());
    if (res == nullptr)
        throw std::runtime_error{"can't allocate a resource for the thread"};

    res->owner = thread_id;
    return res->index;
}

void register_thread(thread_id_t thread_id) noexcept(false)
{
    lock_t lock{protect};
    register_thread(thread_id, lock);
}

void forget_thread(thread_id_t thread_id) noexcept(false)
{
    lock_t lock{protect};

    for (resource_t& res : pool.space)
        if (res.owner == thread_id)
        {
            pool.deallocate(std::addressof(res));
            res.owner = thread_id_t{};
        }

    // unregistered thread. nothing to do
}

uint16_t index_of(thread_id_t thread_id) noexcept(false)
{
    lock_t lock{protect};

    for (resource_t& res : pool.space)
        if (res.owner == thread_id) return res.index;

    // lazy registration
    return register_thread(thread_id, lock);
}

void post_message(thread_id_t thread_id, message_t msg) noexcept(false)
{
    const auto index = index_of(thread_id);
    std::lock_guard<section> lock{queue_lockables[index]};

    auto& queue = queue_list[index];
    queue.push(msg);
}

bool peek_message(message_t& msg) noexcept(false)
{
    thread_id_t thread_id = current_thread_id();

    const auto index = index_of(thread_id);
    std::lock_guard<section> lock{queue_lockables[index]};

    auto& queue = queue_list[index];
    if (queue.empty()) return false;

    msg = std::move(queue.front());
    queue.pop();
    return true;
}
