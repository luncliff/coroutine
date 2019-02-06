// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <array>
#include <condition_variable>

#include "suspend/circular_queue.hpp"
#include "suspend/message_queue.h"
#include "suspend/section.h"

using namespace std;

class lock_cond_queue_t final : public messaging_queue_t
{
    static constexpr auto capacity = 500 + 1;

  private:
    section cs{};
    condition_variable_any cv{};
    bounded_circular_queue_t<message_t> cq{};

  public:
    bool post(message_t msg) noexcept override;
    bool peek(message_t& msg) noexcept override;
    bool wait(message_t& msg, duration timeout) noexcept override;
};

bool lock_cond_queue_t::post(message_t msg) noexcept
{
    unique_lock lck{cs};
    if (cq.push(msg) == true)
    {
        cv.notify_one();
        return true;
    }
    return false;
}

bool lock_cond_queue_t::peek(message_t& msg) noexcept
{
    unique_lock lck{cs};
    return cq.try_pop(msg);
}

bool lock_cond_queue_t::wait(message_t& msg, duration timeout) noexcept
{
    msg = message_t{}; // zero the memory

    unique_lock lck{cs};
    if (cq.empty())
        cv.wait_for(lck, timeout);

    return cq.try_pop(msg);
}

auto create_message_queue() noexcept(false) -> unique_ptr<messaging_queue_t>
{
    return make_unique<lock_cond_queue_t>();
}
