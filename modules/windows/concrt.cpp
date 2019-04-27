//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>
#include <coroutine/frame.h>

#include <gsl/gsl>

#include <atomic>
#include <chrono>
#include <system_error>

#include <Windows.h>
//#include <concrt.h>   // Windows Concurrency Runtime's event is not alertible.
#include <synchapi.h>

#include "stop_watch.hpp"

namespace concrt {
using namespace std;
using namespace std::experimental;
using namespace std::chrono;

win32_event::win32_event() noexcept(false)
    : native{CreateEventA(nullptr, false, false, nullptr)} {
    if (native == INVALID_HANDLE_VALUE)
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "CreateEventA"};
}
win32_event::~win32_event() noexcept {
    this->close();
}
bool win32_event::is_closed() const noexcept {
    return native == INVALID_HANDLE_VALUE;
}
void win32_event::close() noexcept {
    CloseHandle(native);
    native = INVALID_HANDLE_VALUE;
}
void win32_event::reset() noexcept {
    ResetEvent(native);
}
void win32_event::set() noexcept {
    SetEvent(native);
}
bool win32_event::wait(uint32_t ms) noexcept {
    static_assert(WAIT_OBJECT_0 == 0);

    // This makes APC available. expecially for Overlapped I/O
    if (WaitForSingleObjectEx(native, ms, TRUE))
        // WAIT_FAILED	: use GetLastError in the case
        // WAIT_TIMEOUT	: this is expected. user can try again
        // WAIT_IO_COMPLETION : return because of APC
        // WAIT_ABANDONED
        return false;

    // WAIT_OBJECT_0 : return by signal
    this->close();
    return true;
}

latch::latch(uint32_t delta) noexcept(false) : ev{}, ref{delta} {
    ev.reset();
}

void latch::count_down_and_wait() noexcept(false) {
    this->count_down();
    this->wait();
}
void latch::count_down(uint32_t n) noexcept(false) {

    if (ref.load(memory_order_acquire) < n)
        throw underflow_error{"latch's count can't be negative"};

    ref.fetch_sub(n, memory_order_release);
    if (ref.load(memory_order_acquire) == 0)
        ev.set();
}
GSL_SUPPRESS(f .23)
GSL_SUPPRESS(con .4)
bool latch::is_ready() const noexcept {
    if (ref.load(memory_order_acquire) == 0)
        // if it is not closed, test it
        return ev.is_closed() ? true : ev.wait(0);
    return false;
}
void latch::wait() noexcept(false) {
    // standard interface doesn't define timed wait.
    while (ev.wait(INFINITE) == false)
        ;
}

void ptp_work::resume_on_thread_pool(PTP_CALLBACK_INSTANCE, //
                                     PVOID ctx, PTP_WORK work) {
    if (auto coro = coroutine_handle<void>::from_address(ctx))
        if (coro.done() == false)
            coro.resume();

    ::CloseThreadpoolWork(work); // one-time work item
}
auto ptp_work::suspend(coroutine_handle<void> coro) noexcept -> uint32_t {
    // just make sure no data loss in `static_cast`
    static_assert(sizeof(uint32_t) == sizeof(DWORD));

    auto work =
        ::CreateThreadpoolWork(resume_on_thread_pool, coro.address(), nullptr);
    if (work == nullptr)
        return GetLastError();

    SubmitThreadpoolWork(work);
    return S_OK;
}

section::section() noexcept(false) {
    InitializeCriticalSectionAndSpinCount(this, 0600);
}
section::~section() noexcept {
    DeleteCriticalSection(this);
}
bool section::try_lock() noexcept {
    return TryEnterCriticalSection(this);
}
void section::lock() noexcept(false) {
    EnterCriticalSection(this);
}
void section::unlock() noexcept(false) {
    LeaveCriticalSection(this);
}
} // namespace concrt