//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>
#include <coroutine/frame.h>

#include <atomic>
#include <chrono>
#include <system_error>

#include <Windows.h>
//#include <concrt.h>   // Windows Concurrency Runtime's event is not alertible.
#include <synchapi.h>

#include "stop_watch.hpp"

namespace concrt
{
using namespace std;
using namespace std::experimental;
using namespace std::chrono;

class event_type
{
    HANDLE ev{};

  public:
    event_type(const event_type&) = delete;
    event_type(event_type&&) = delete;
    event_type& operator=(const event_type&) = delete;
    event_type& operator=(event_type&&) = delete;

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

        // This makes APC available.
        // expecially for Overlapped I/O
        DWORD ec = WaitForSingleObjectEx(ev, ms, TRUE);

        if (ec == WAIT_FAILED)
            return false;

        // the only case for success
        else if (ec == WAIT_OBJECT_0)
        {
            CloseHandle(ev);
            ev = INVALID_HANDLE_VALUE;
            return true;
        }
        // timeout. this is expected failure
        // the user code can try again
        else if (ec == WAIT_TIMEOUT)
            return false;
        // return because of APC
        else if (ec == WAIT_IO_COMPLETION)
            return false;

        // WAIT_ABANDONED
        return false; // user will check the error again
    }
};

struct latch_win32 final
{
    atomic<uint32_t> count{};
    event_type event{};
};

auto for_win32(latch* lc) noexcept
{
    static_assert(sizeof(latch_win32) < sizeof(latch));
    return reinterpret_cast<latch_win32*>(lc);
}
auto for_win32(const latch* lc) noexcept
{
    return for_win32(const_cast<latch*>(lc));
}

latch::latch(uint32_t count) noexcept(false) : storage{}
{
    auto im = new (for_win32(this)) latch_win32{};
    im->count.fetch_add(count);
    im->event.reset();
}

latch::~latch() noexcept
{
    auto im = for_win32(this);
    im->~latch_win32();
}

void latch::count_down_and_wait() noexcept(false)
{
    this->count_down();
    this->wait();
}
void latch::count_down(uint32_t n) noexcept(false)
{
    auto im = for_win32(this);
    if (im->count < n)
        throw underflow_error{"latch's count can't be negative"};

    im->count.fetch_sub(n, memory_order_release);
    if (im->count.load(memory_order_acquire) == 0)
        im->event.set();
}

bool latch::is_ready() const noexcept
{
    if (auto im = for_win32(this))
        return im->count == 0 && im->event.wait(0) == true;
    return false;
}
void latch::wait() noexcept(false)
{
    auto im = for_win32(this);
    while (im->event.wait(INFINITE) == false)
        ;
}

void ptp_work::resume_on_thread_pool(PTP_CALLBACK_INSTANCE, //
                                     PVOID ctx, PTP_WORK work)
{
    if (auto coro = coroutine_handle<void>::from_address(ctx))
        if (coro.done() == false)
            coro.resume();

    ::CloseThreadpoolWork(work); // one-time work item
}

auto ptp_work::suspend(coroutine_handle<void> coro) noexcept -> errc
{
    // just make sure no data loss in `static_cast`
    static_assert(sizeof(errc) >= sizeof(DWORD));

    auto work = ::CreateThreadpoolWork(resume_on_thread_pool, coro.address(),
                                       nullptr);
    if (work == nullptr)
        return static_cast<errc>(GetLastError());

    SubmitThreadpoolWork(work);
    return errc{};
}

} // namespace concrt