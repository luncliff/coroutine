// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------

#define NOMINMAX

#include <array>
#include <cassert>
#include <queue>
#include <system_error>

#include <coroutine/sync.h>

constexpr uint16_t max_thread_count = 300;

static_assert(std::atomic<message_t>::is_always_lock_free);
using queue_t = std::queue<message_t>;

std::array<section, max_thread_count> queue_lockables{};
std::array<queue_t, max_thread_count> queue_list{};


extern uint16_t index_of(uint32_t thread_id) noexcept(false);

void post_message(uint32_t thread_id, message_t msg) noexcept(false)
{
    const auto index = index_of(thread_id);
    std::lock_guard<section> lock{queue_lockables[index]};

    auto& queue = queue_list[index];
    queue.push(msg);
}

bool peek_message(uint32_t thread_id, message_t& msg) noexcept(false)
{
    const auto index = index_of(thread_id);
    std::lock_guard<section> lock{queue_lockables[index]};

    auto& queue = queue_list[index];
    if (queue.empty()) return false;

    msg = std::move(queue.front());
    queue.pop();
    return true;
}
