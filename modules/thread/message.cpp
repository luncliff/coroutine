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
#include <unordered_map>

#include "../memory.hpp"
#include <coroutine/sync.h>

// over this, there is too much thread !
constexpr uint16_t max_thread_count = 100;

struct resource_t
{
    uint64_t owner;
    uint16_t mark;
    uint16_t index;
};

section protect{};
index_pool<resource_t, max_thread_count> pool{};

void setup_indices() noexcept
{
    uint16_t i = 0;
    for (auto& res : pool.space)
        res.index = i++;
}

void teardown_indices() noexcept
{
    for (auto& res : pool.space)
        if (res.owner) pool.deallocate(std::addressof(res));
}

uint16_t register_thread(uint32_t thread_id,
                         const std::unique_lock<section>&) noexcept(false)
{
    auto* res = reinterpret_cast<resource_t*>(pool.allocate());
    if (res == nullptr)
        throw std::runtime_error{"can't allocate a resource for the thread"};

    res->owner = thread_id;
    return res->index;
}

void register_thread(uint64_t thread_id) noexcept(false)
{
    std::unique_lock lck{protect};
    register_thread(thread_id, lck);
}

void forget_thread(uint64_t thread_id) noexcept(false)
{
    std::unique_lock lck{protect};

    for (resource_t& res : pool.space)
        if (res.owner == thread_id)
        {
            pool.deallocate(std::addressof(res));
            res.owner = 0;
        }

    // unregistered thread. nothing to do
}

uint16_t index_of(uint64_t thread_id) noexcept(false)
{
    std::unique_lock lck{protect};
    // simple linear search
    for (resource_t& res : pool.space)
        if (res.owner == thread_id) // expect match here
            return res.index;

    // lazy registration
    return register_thread(thread_id, lck);
}

using queue_t = std::queue<message_t>;

std::array<section, max_thread_count> queue_lockables{};
std::array<queue_t, max_thread_count> queue_list{};

extern uint16_t index_of(uint64_t thread_id) noexcept(false);

void post_message(uint64_t thread_id, const message_t msg) noexcept(false)
{
    const auto index = index_of(thread_id);
    std::unique_lock lck{queue_lockables[index]};

    auto& queue = queue_list[index];
    queue.push(msg);
}

bool peek_message(uint64_t thread_id, message_t& msg) noexcept(false)
{
    const auto index = index_of(thread_id);
    std::unique_lock lck{queue_lockables[index]};

    auto& queue = queue_list[index];
    if (queue.empty())
        // empty. peek failed
        return false;

    msg = std::move(queue.front());
    queue.pop();
    return true;
}
