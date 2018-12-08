// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>
#include <messaging/concurrent.h>
#include <thread/types.h>

#include <cassert>
#include <thread>

using namespace std;
using namespace std::chrono;

extern thread_registry registry;

void lazy_delivery(thread_id_t thread_id, message_t msg) noexcept(false);

bool post_message(thread_id_t thread_id, message_t msg) noexcept(false)
{
    auto* info = registry.search(static_cast<uint64_t>(thread_id));
    if (info == nullptr)
        throw std::runtime_error{"sending message to unregisted thread id"};

    thread_data* receiver = *info;

    // the given thread is registered, but its queue is not.
    if (receiver == nullptr)
    {
        lazy_delivery(thread_id, msg);
        return true;
    }

    return receiver->queue.push(msg);
}

bool peek_message(message_t& msg) noexcept(false)
{
    return get_local_data()->queue.try_pop(msg);
}

bool get_message(message_t& msg, nanoseconds timeout) noexcept(false)
{
    if (peek_message(msg))
        return true;

    // this is the most simple implementation.
    // just try again after timeout.

    // need conditional variable to be correct
    this_thread::sleep_for(timeout);
    return peek_message(msg);
}
