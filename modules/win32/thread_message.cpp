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

#include <array>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NOCRYPT
#include <Windows.h>

static_assert(std::atomic<message_t>::is_always_lock_free);

struct circular_message_queue final
{
    // this won't be enough for some scenario.
    // but busy polling might be enough in most case
    static constexpr auto capacity = 500;
    static constexpr auto invalid_index = std::numeric_limits<uint16_t>::max();

    using value_type = message_t;
    using index_type = uint16_t;

    index_type begin = 0, end = 0;
    size_t count = 0;
    std::array<value_type, capacity> storage{};

  private:
    static index_type advance(index_type i) noexcept
    {
        return (i + 1) % capacity;
    }

  public:
    circular_message_queue() noexcept = default;

  public:
    bool is_full() noexcept { return count == storage.max_size(); }
    bool is_empty() noexcept { return count == 0; }

    bool push(const value_type msg)
    {
        if (is_full()) return false;

        auto index = end;
        end = advance(end);
        count += 1;

        storage[index] = msg;
        return true;
    }

    [[nodiscard]] bool pop(value_type& msg) {
        if (is_empty()) return false;

        auto index = begin;
        begin = advance(begin);
        count -= 1;

        msg = std::move(storage[index]);
        return true;
    }
};

thread_local circular_message_queue tl_msg_queue{};

// Prevent race with APC callback
void CALLBACK deliver(const message_t msg) noexcept
{
    static_assert(sizeof(message_t) == sizeof(ULONG_PTR));

    if (tl_msg_queue.push(msg) == false) SetLastError(ERROR_NOT_ENOUGH_QUOTA);
}

// follow the scheme of `PostThreadMessageW` ?
void post_message(uint32_t thread_id, const message_t msg) noexcept(false)
{
    int error = S_OK;
    HANDLE thread = INVALID_HANDLE_VALUE;
    DWORD success = 0;

    SleepEx(0, true); // handle some APC

    thread = OpenThread(THREAD_SET_CONTEXT, FALSE, thread_id);
    if (thread == NULL)
    {
        error = GetLastError();
        goto OnSysError;
    }

    // use apc queue to deliver message
    success = QueueUserAPC( // !!! need performance check !!!
        reinterpret_cast<PAPCFUNC>(deliver),
        thread,
        msg.u64);
    error = GetLastError();

    CloseHandle(thread);  // delete the handle
    if (success == false) // non-zero if successful
        goto OnSysError;

    return;
OnSysError:
    throw std::error_code{error, std::system_category()};
}

// Do this function need to follow the the scheme of `PeekMessageW` ?
// fetch from thread_local queue
bool peek_message(message_t& msg) noexcept(false)
{
    bool fetched = tl_msg_queue.pop(msg);
    // handle some APC
    SleepEx(0, true);
    return fetched;
}
