// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <messaging/concurrent.h>
#include <thread/types.h>

#include <thread>

extern thread_registry registry;
extern thread_local thread_data current_data;

bool post_message(thread_id_t thread_id, message_t msg) noexcept(false)
{
    thread_data** info = registry.search(static_cast<uint64_t>(thread_id));
    if (info == nullptr)
        throw std::runtime_error{"sending message to unregisted thread id"};

    thread_data* receiver = *info;
    return receiver->queue.push(msg);
}

bool peek_message(message_t& msg) noexcept(false)
{
    return current_data.queue.try_pop(msg);
}

bool get_message(message_t& msg,
                 std::chrono::nanoseconds timeout) noexcept(false)
{
    if (peek_message(msg))
        return true;

    // this is the most simple implementation.
    // just try again after timeout.

    // need conditional variable to be correct
    std::this_thread::sleep_for(timeout);
    return peek_message(msg);
}
