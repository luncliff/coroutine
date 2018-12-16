// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/sync.h>
#include <messaging/concurrent.h>
#include <thread/types.h>

#include <cassert>
#include <thread>

using namespace std;
using namespace std::chrono;

void lazy_delivery(thread_id_t thread_id, message_t msg) noexcept(false);

bool post_message(thread_id_t tid, message_t msg) noexcept(false)
{
    auto* info = get_thread_registry()->find_or_insert(tid);

    thread_data* receiver = *info;
    // the given thread is registered,
    // but its queue is not initialized
    if (receiver == nullptr)
    {
        // throw if something goes wrong
        lazy_delivery(tid, msg);
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
