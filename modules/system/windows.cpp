//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//

// clang-format off
#include <Windows.h>
#include <TlHelp32.h>
#include <threadpoolapiset.h>
#include <synchapi.h>
// Windows Concurrency Runtime's event is not alertible.
//#include <concrt.h>
// clang-format on

#include <coroutine/windows.h>
#include <gsl/gsl>

#include <cassert>
#include <system_error>

using namespace std;
using namespace gsl;

namespace coro {

static_assert(is_move_assignable_v<set_or_cancel> == false);
static_assert(is_move_constructible_v<set_or_cancel> == false);
static_assert(is_copy_assignable_v<set_or_cancel> == false);
static_assert(is_copy_constructible_v<set_or_cancel> == false);

GSL_SUPPRESS(con .4)
void __stdcall wait_event_on_thread_pool(PVOID ctx, BOOLEAN timedout) {
    UNREFERENCED_PARAMETER(timedout);
    auto coro = coroutine_handle<void>::from_address(ctx);
    assert(coro.done() == false);
    coro.resume();
}

set_or_cancel::set_or_cancel(HANDLE target_event) noexcept(false)
    : hobject{target_event} {
    // wait object is used as a storage for the event handle
    // until it is going to suspend
}
set_or_cancel::~set_or_cancel() noexcept {
    UnregisterWait(hobject);
}

auto set_or_cancel::unregister() noexcept -> uint32_t {
    UnregisterWait(hobject);
    const auto ec = GetLastError();
    if (ec == ERROR_IO_PENDING) {
        // this is expected since we are using INFINITE timeout
        return NO_ERROR;
    }
    return ec;
}

void set_or_cancel::suspend(coroutine_handle<void> coro) noexcept(false) {
    // since this point, wo becomes a handle for the request
    // this is one-shot event. so use infinite timeout
    if (RegisterWaitForSingleObject(addressof(hobject), hobject,
                                    wait_event_on_thread_pool, coro.address(),
                                    INFINITE, WT_EXECUTEONLYONCE) == FALSE) {
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "RegisterWaitForSingleObject"};
    }
}

// auto get_threads_of(DWORD pid) noexcept(false) -> enumerable<DWORD> {
//     // for current process
//     auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
//     if (snapshot == INVALID_HANDLE_VALUE)
//         co_return;
//     auto on_return = gsl::finally([=]() noexcept { CloseHandle(snapshot); });
//     THREADENTRY32 entry{};
//     entry.dwSize = sizeof(THREADENTRY32);
//     for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);) {
//         // filter other process's threads
//         if (entry.th32OwnerProcessID != pid)
//             co_yield entry.th32ThreadID;
//         entry.dwSize = sizeof(THREADENTRY32);
//     }
// }

GSL_SUPPRESS(con .4)
void continue_on_thread_pool::resume_on_thread_pool(PTP_CALLBACK_INSTANCE, //
                                                    PVOID ctx, PTP_WORK work) {
    if (auto coro = coroutine_handle<void>::from_address(ctx))
        if (coro.done() == false)
            coro.resume();

    ::CloseThreadpoolWork(work); // one-time work item
}

auto continue_on_thread_pool::create_and_submit_work(
    coroutine_handle<void> coro) noexcept -> uint32_t {
    // just make sure no data loss in `static_cast`
    static_assert(sizeof(uint32_t) == sizeof(DWORD));

    auto work = ::CreateThreadpoolWork(resume_on_thread_pool, //
                                       coro.address(), nullptr);
    if (work == nullptr)
        return GetLastError();

    SubmitThreadpoolWork(work);
    return S_OK;
}

GSL_SUPPRESS(type .1)
void continue_on_apc::resume_on_apc(ULONG_PTR param) {
    auto ptr = reinterpret_cast<void*>(param);
    if (auto coro = coroutine_handle<void>::from_address(ptr))
        coro.resume();
}

GSL_SUPPRESS(type .1)
auto continue_on_apc::queue_user_apc(coroutine_handle<void> coro) noexcept
    -> uint32_t {
    const auto param = reinterpret_cast<ULONG_PTR>(coro.address());

    if (QueueUserAPC(resume_on_apc, this->thread, param) == 0)
        return GetLastError();
    return S_OK;
}

} // namespace coro
