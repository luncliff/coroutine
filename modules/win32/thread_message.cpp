// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <array>
#include <cassert>
#include <queue>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOCRYPT
#include <Windows.h>

static_assert(std::atomic<message_t>::is_always_lock_free);

struct apc_msg_queue final
{
    void push(const message_t msg) {}
    bool empty() {}

    void pop(message_t& msg) {}
};

thread_local apc_msg_queue amq{};

void WINAPI deliver(const message_t msg) noexcept { amq.push(msg); }

static_assert(sizeof(message_t) == sizeof(ULONG_PTR));

void post_message(uint32_t thread_id, const message_t msg) noexcept(false)
{
    SleepEx(0, true); // handle some APC

    int error = S_OK;
    // make a handle th thread
    auto thread = OpenThread(THREAD_SET_CONTEXT, FALSE, thread_id);
    // use apc queue to deliver message
    auto code =
        QueueUserAPC(reinterpret_cast<PAPCFUNC>(deliver), thread, msg.u64);

    error = GetLastError(); // get error code before another syscall
    CloseHandle(thread);    // delete the handle
    if (code == 0)          // non-zero if successful
        throw std::error_code{error, std::system_category()};
}

bool peek_message(uint32_t thread_id, message_t& msg) noexcept(false)
{
    SleepEx(0, true); // handle some APC

    // thread?
    if (amq.empty()) return false;

    amq.pop(msg);
}
