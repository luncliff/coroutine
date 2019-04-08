//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concurrency_adapter.h>

#include <atomic>
#include <chrono>
#include <system_error>

#include <Windows.h>
//#include <concrt.h>   // Windows Concurrency Runtime's event is not alertible.
#include <synchapi.h>

#include "stop_watch.hpp"

namespace cc
{
using namespace std;
using namespace std::chrono;

struct event_type
{
    HANDLE ev{};

    event_type() noexcept(false)
        : ev{CreateEventA(nullptr, false, false, nullptr)}
    {
        if (ev == INVALID_HANDLE_VALUE)
            throw system_error{static_cast<int>(GetLastError()),
                               system_category(), "CreateEventA"};
    }
    ~event_type() noexcept
    {
        if (ev == INVALID_HANDLE_VALUE)
            return;
        CloseHandle(ev);
    }

    void reset() noexcept
    {
        ResetEvent(ev);
    }
    void set() noexcept
    {
        SetEvent(ev);
    }
    bool wait(uint32_t ms) noexcept
    {
        if (ev == INVALID_HANDLE_VALUE)
            return true;
    TryWait:
        // This makes APC available.
        // expecially for Overlapped I/O
        DWORD ec = WaitForSingleObjectEx(ev, ms, TRUE);

        if (ec == WAIT_FAILED)
            return false;

        // the only case for success
        else if (ec == WAIT_OBJECT_0)
        {
            auto h = ev;
            ev = INVALID_HANDLE_VALUE;
            CloseHandle(h);
            return true;
        }

        // timeout. this is expected failure
        // the user code can try again
        else if (ec == WAIT_TIMEOUT)
            return false;

        // return because of APC
        else if (ec == WAIT_IO_COMPLETION)
            goto TryWait;

        return false; // user will check the error again
    }
};

struct latch_win32 final
{
    atomic<uint32_t> count{};
    event_type event{};
};

constexpr auto cast(latch* lc) noexcept
{
    static_assert(sizeof(latch_win32) < sizeof(latch));
    return reinterpret_cast<latch_win32*>(lc);
}
constexpr auto cast(const latch* lc) noexcept
{
    return cast(const_cast<latch*>(lc));
}

latch::latch(uint32_t count) noexcept(false) : storage{}
{
    auto im = new (cast(this)) latch_win32{};
    im->count.fetch_add(count);
    im->event.reset();
}

latch::~latch() noexcept
{
    auto im = cast(this);
    im->~latch_win32();
}

void latch::count_down_and_wait() noexcept(false)
{
    this->count_down();
    this->wait();
}
void latch::count_down(uint32_t n) noexcept(false)
{
    auto im = cast(this);
    if (im->count < n)
        throw underflow_error{"latch's count can't be negative"};

    im->count.fetch_sub(n, memory_order_release);
    if (im->count.load(memory_order_acquire) == 0)
        im->event.set();
}

bool latch::is_ready() const noexcept
{
    if (auto im = cast(this))
        return im->count == 0 && im->event.wait(0) == true;
    return false;
}
void latch::wait() noexcept(false)
{
    auto im = cast(this);
    while (im->event.wait(INFINITE) == false)
        ;
}

using barrier_win32 = ::SYNCHRONIZATION_BARRIER;

constexpr auto cast(barrier* lc) noexcept
{
    static_assert(sizeof(barrier_win32) < sizeof(barrier));
    return reinterpret_cast<barrier_win32*>(lc);
}
constexpr auto cast(const barrier* lc) noexcept
{
    return cast(const_cast<barrier*>(lc));
}

barrier::barrier(uint32_t num_threads) noexcept(false) : storage{}
{
    auto self = new (cast(this)) barrier_win32{};
    // https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-initializesynchronizationbarrier
    if (::InitializeSynchronizationBarrier(self, num_threads, 1600) == FALSE)
        throw system_error{static_cast<int>(::GetLastError()),
                           system_category(),
                           "InitializeSynchronizationBarrier"};
}
barrier::~barrier() noexcept
{
    // https://docs.microsoft.com/en-us/windows/desktop/api/SynchAPI/nf-synchapi-deletesynchronizationbarrier
    ::DeleteSynchronizationBarrier(cast(this));
}

void barrier::arrive_and_wait() noexcept
{
    // https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-entersynchronizationbarrier
    ::EnterSynchronizationBarrier(cast(this),
                                  SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
}

} // namespace cc