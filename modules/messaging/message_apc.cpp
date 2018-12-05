// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Note
//      I/O Completion Port can be a good choice for thread messaging.
//      But the purpose of this implementation is to spend more time in
//      user-level. Unless there is an issue about performance
//      or consistency, APC will do the works
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <gsl/gsl_util>

#include <system_error>

#include <Windows.h>
#include <concurrent_queue.h>

thread_local concurrency::concurrent_queue<message_t> msg_queue{};

// Prevent race with APC callback
void CALLBACK deliver(const message_t msg) noexcept
{
    static_assert(sizeof(message_t) == sizeof(ULONG_PTR));

    return msg_queue.push(msg);
}

// follow the scheme of `PostThreadMessageW` ?
void post_message(uint32_t thread_id, const message_t msg) noexcept(false)
{
    SleepEx(0, true); // handle some APC

    // receiver thread
    HANDLE thread = OpenThread(THREAD_SET_CONTEXT, FALSE, thread_id);
    if (thread == NULL)
        throw std::system_error{gsl::narrow_cast<int>(GetLastError()),
                                std::system_category(), "OpenThread"};
    // just ensure close
    auto h = gsl::finally([=]() { CloseHandle(thread); });

    // use apc queue to deliver message
    auto success
        = QueueUserAPC(reinterpret_cast<PAPCFUNC>(deliver), thread, msg.u64);

    if (success == false)
        // non-zero if successful
        throw std::system_error{gsl::narrow_cast<int>(GetLastError()),
                                std::system_category(), "QueueUserAPC"};
}

// Do this function need to follow the the scheme of `PeekMessageW` ?
// fetch from thread_local queue
bool peek_message(message_t& msg) noexcept(false)
{
    auto fetched = msg_queue.try_pop(msg);
    // handle some APC
    SleepEx(0, true);
    return fetched;
}
