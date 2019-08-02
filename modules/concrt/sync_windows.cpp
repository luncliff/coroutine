//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
//
#include <concurrency_helper.h>
#include <coroutine/frame.h>
#include <system_error>

#include <TlHelp32.h>
#include <gsl/gsl>

// Windows Concurrency Runtime's event is not alertible.
//#include <concrt.h>

using namespace std;
using namespace gsl;

system_error make_sys_error(not_null<czstring<>> label) noexcept(false) {
    const auto ec = gsl::narrow_cast<int>(GetLastError());
    return system_error{ec, system_category(), label};
}

auto get_threads(DWORD pid) noexcept(false) -> coro::enumerable<DWORD> {
    // for current process
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw make_sys_error("CreateToolhelp32Snapshot");

    auto h = gsl::finally([=]() noexcept { CloseHandle(snapshot); });

    auto entry = THREADENTRY32{};
    entry.dwSize = sizeof(entry);

    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);
         entry.dwSize = sizeof(entry)) {
        // filter other process's threads
        if (entry.th32OwnerProcessID != pid)
            co_yield entry.th32ThreadID;
    }
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

static_assert(is_move_assignable_v<latch> == false);
static_assert(is_move_constructible_v<latch> == false);
static_assert(is_copy_assignable_v<latch> == false);
static_assert(is_copy_constructible_v<latch> == false);

latch::latch(uint32_t delta) noexcept(false)
    : ev{CreateEventEx(nullptr, nullptr, //
                       CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS)},
      ref{delta} {
    if (ev == INVALID_HANDLE_VALUE)
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "CreateEventEx"};

    ResetEvent(ev);
}
latch::~latch() noexcept {
    if (ev != INVALID_HANDLE_VALUE) {
        CloseHandle(ev);
        ev = INVALID_HANDLE_VALUE;
    }
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
        this->~latch();
        return true;
    }
    return false;
}

GSL_SUPPRESS(es .76)
void latch::wait() noexcept(false) {
    static_assert(WAIT_OBJECT_0 == 0);

StartWait:
    if (this->is_ready())
        return;

    // standard interface doesn't define timed wait.
    // This makes APC available. expecially for Overlapped I/O
    if (const auto ec = WaitForSingleObjectEx(ev, 1536, TRUE)) {
        // WAIT_IO_COMPLETION : return because of APC
        if (ec == WAIT_IO_COMPLETION)
            goto StartWait;
        // WAIT_TIMEOUT	: this is expected. try again
        if (ec == WAIT_TIMEOUT)
            goto StartWait;

        // WAIT_FAILED	: use GetLastError in the case
        // WAIT_ABANDONED
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "WaitForSingleObjectEx"};
    }
    // WAIT_OBJECT_0 : return by signal
    this->~latch();
    return;
}
