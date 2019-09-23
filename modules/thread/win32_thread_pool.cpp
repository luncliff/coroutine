//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>
#include <coroutine/yield.hpp>

#include <gsl/gsl>

#include <TlHelp32.h>
#include <synchapi.h>
//#include <threadpoolapiset.h>

namespace coro {

auto get_threads_of(DWORD pid) noexcept(false) -> enumerable<DWORD> {
    // for current process
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        co_return;

    auto h = gsl::finally([=]() noexcept { CloseHandle(snapshot); });

    THREADENTRY32 entry{};
    entry.dwSize = sizeof(THREADENTRY32);

    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);) {
        // filter other process's threads
        if (entry.th32OwnerProcessID != pid)
            co_yield entry.th32ThreadID;

        entry.dwSize = sizeof(THREADENTRY32);
    }
}

GSL_SUPPRESS(con .4)
void ptp_work::resume_on_thread_pool(PTP_CALLBACK_INSTANCE, //
                                     PVOID ctx, PTP_WORK work) {
    if (auto coro = coroutine_handle<void>::from_address(ctx))
        if (coro.done() == false)
            coro.resume();

    ::CloseThreadpoolWork(work); // one-time work item
}

auto ptp_work::create_and_submit_work(coroutine_handle<void> coro) noexcept
    -> uint32_t {
    // just make sure no data loss in `static_cast`
    static_assert(sizeof(uint32_t) == sizeof(DWORD));

    auto work =
        ::CreateThreadpoolWork(resume_on_thread_pool, coro.address(), nullptr);
    if (work == nullptr)
        return GetLastError();

    SubmitThreadpoolWork(work);
    return S_OK;
}

GSL_SUPPRESS(type .1)
void procedure_call_on::resume_on_apc(ULONG_PTR param) {
    auto ptr = reinterpret_cast<void*>(param);
    if (auto coro = coroutine_handle<void>::from_address(ptr))
        coro.resume();
}

GSL_SUPPRESS(type .1)
auto procedure_call_on::queue_user_apc(coroutine_handle<void> coro) noexcept
    -> uint32_t {
    const auto param = reinterpret_cast<ULONG_PTR>(coro.address());

    if (QueueUserAPC(resume_on_apc, this->thread, param) == 0)
        return GetLastError();
    return S_OK;
}

} // namespace coro
