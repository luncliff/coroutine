//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>
#include <coroutine/frame.h>

#include <gsl/gsl>

#include <system_error>

#include <Windows.h>
//#include <concrt.h>   // Windows Concurrency Runtime's event is not alertible.
#include <synchapi.h>

using namespace std;
using namespace gsl;

extern system_error make_sys_error(not_null<czstring<>> label) noexcept(false);

namespace concrt {

static_assert(is_move_assignable_v<ptp_event> == false);
static_assert(is_move_constructible_v<ptp_event> == false);
static_assert(is_copy_assignable_v<ptp_event> == false);
static_assert(is_copy_constructible_v<ptp_event> == false);

GSL_SUPPRESS(con .4)
void __stdcall ptp_event::wait_on_thread_pool(PVOID ctx, BOOLEAN timedout) {
    // we are using INFINITE
    UNREFERENCED_PARAMETER(timedout);
    auto coro = coroutine_handle<void>::from_address(ctx);
    assert(coro.done() == false);
    coro.resume();
}
ptp_event::ptp_event(HANDLE ev) noexcept(false) : wo{ev} {
    // wait object is used as a storage for the event handle
    // until it is going to suspend
}
ptp_event::~ptp_event() noexcept {
    if (wo != INVALID_HANDLE_VALUE)
        this->cancel();
}
void ptp_event::cancel() noexcept {
    this->on_resume();
}
bool ptp_event::is_ready() const noexcept {
    return false;
}
// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-registerwaitforsingleobject
// consider: can we use WT_EXECUTEINWAITTHREAD for this type?
void ptp_event::on_suspend(coroutine_handle<void> coro) noexcept(false) {
    // since this point, wo becomes a handle for the request
    auto ev = wo;
    // this is one-shot event. so use infinite timeout
    if (RegisterWaitForSingleObject(addressof(wo), ev, wait_on_thread_pool,
                                    coro.address(), INFINITE,
                                    WT_EXECUTEONLYONCE) == FALSE)
        throw make_sys_error("RegisterWaitForSingleObject");
}
// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-unregisterwait
uint32_t ptp_event::on_resume() noexcept {
    DWORD ec = NO_ERROR;
    if (wo == INVALID_HANDLE_VALUE)
        return ec;

    if (UnregisterWait(wo) == false)
        ec = GetLastError();

    // this is expected since we are using INFINITE timeout
    if (ec == ERROR_IO_PENDING)
        ec = NO_ERROR;

    wo = INVALID_HANDLE_VALUE;
    return ec;
}

static_assert(is_move_assignable_v<latch> == false);
static_assert(is_move_constructible_v<latch> == false);
static_assert(is_copy_assignable_v<latch> == false);
static_assert(is_copy_constructible_v<latch> == false);

latch::latch(uint32_t delta) noexcept(false)
    : ev{CreateEventEx(nullptr, nullptr, //
                       CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS)},
      ref{delta} {
    if (ev == INVALID_HANDLE_VALUE)
        throw make_sys_error("CreateEventA");
    ResetEvent(ev);
}
latch::~latch() noexcept {
    if (ev != INVALID_HANDLE_VALUE)
        CloseHandle(ev);
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
        SetEvent(ev);
}
GSL_SUPPRESS(f .23)
GSL_SUPPRESS(con .4)
bool latch::is_ready() const noexcept {
    if (ref.load(memory_order_acquire) > 0)
        return false;

    if (ev == INVALID_HANDLE_VALUE)
        return true;

    // if it is not closed, test it
    auto ec = WaitForSingleObjectEx(ev, 0, TRUE);
    if (ec == WAIT_OBJECT_0) {
        // WAIT_OBJECT_0 : return by signal
        CloseHandle(ev);
        ev = INVALID_HANDLE_VALUE;
        return true;
    }
    return false;
}
void latch::wait() noexcept(false) {
    constexpr auto timeout = INFINITE;
    static_assert(WAIT_OBJECT_0 == 0);

    // standard interface doesn't define timed wait.
    // This makes APC available. expecially for Overlapped I/O
    if (WaitForSingleObjectEx(ev, timeout, TRUE))
        // WAIT_FAILED	: use GetLastError in the case
        // WAIT_TIMEOUT	: this is expected. user can try again
        // WAIT_IO_COMPLETION : return because of APC
        // WAIT_ABANDONED
        return;

    // WAIT_OBJECT_0 : return by signal
    CloseHandle(ev);
    ev = INVALID_HANDLE_VALUE;
    return;
}

GSL_SUPPRESS(con .4)
void ptp_work::resume_on_thread_pool(PTP_CALLBACK_INSTANCE, //
                                     PVOID ctx, PTP_WORK work) {
    if (auto coro = coroutine_handle<void>::from_address(ctx))
        if (coro.done() == false)
            coro.resume();

    ::CloseThreadpoolWork(work); // one-time work item
}
auto ptp_work::on_suspend(coroutine_handle<void> coro) noexcept -> uint32_t {
    // just make sure no data loss in `static_cast`
    static_assert(sizeof(uint32_t) == sizeof(DWORD));

    auto work =
        ::CreateThreadpoolWork(resume_on_thread_pool, coro.address(), nullptr);
    if (work == nullptr)
        return GetLastError();

    SubmitThreadpoolWork(work);
    return S_OK;
}

static_assert(is_move_assignable_v<section> == false);
static_assert(is_move_constructible_v<section> == false);
static_assert(is_copy_assignable_v<section> == false);
static_assert(is_copy_constructible_v<section> == false);

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