// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

#define NOMINMAX

#include <array>
#include <cassert>
#include <system_error>
#include <unordered_map>

#include <coroutine/sync.h>

#include "../memory.hpp"

using lock_t = std::lock_guard<section>;

constexpr uint16_t max_thread_count = 60;

struct resource_t
{
    uint32_t owner;
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

[[noreturn]] void teardown_indices() noexcept
{
    for (auto& res : pool.space)
        if (res.owner) pool.deallocate(std::addressof(res));
}

uint16_t register_thread(uint32_t thread_id, const lock_t&) noexcept(false)
{
    auto* res = reinterpret_cast<resource_t*>(pool.allocate());
    if (res == nullptr)
        throw std::runtime_error{"can't allocate a resource for the thread"};

    res->owner = thread_id;
    return res->index;
}

void register_thread(uint32_t thread_id) noexcept(false)
{
    lock_t lock{protect};
    register_thread(thread_id, lock);
}

void forget_thread(uint32_t thread_id) noexcept(false)
{
    lock_t lock{protect};

    for (resource_t& res : pool.space)
        if (res.owner == thread_id)
        {
            pool.deallocate(std::addressof(res));
            res.owner = 0;
        }

    // unregistered thread. nothing to do
}

uint16_t index_of(uint32_t thread_id) noexcept(false)
{
    lock_t lock{protect};

    for (resource_t& res : pool.space)
        if (res.owner == thread_id) return res.index;

    // lazy registration
    return register_thread(thread_id, lock);
}
