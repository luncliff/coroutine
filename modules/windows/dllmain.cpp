// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
//
// ---------------------------------------------------------------------------
#include <coroutine/enumerable.hpp>
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <gsl/gsl_util>

// clang-format off
#include <Windows.h>
#include <sdkddkver.h>
#include <TlHelp32.h>
// clang-format on

using namespace std;

thread_id_t current_thread_id() noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

bool check_thread_exists(thread_id_t id) noexcept
{
    // if the thread exists, OpenThread will be successful
    if (HANDLE thread = OpenThread(
            THREAD_SET_CONTEXT, FALSE, static_cast<DWORD>(id)))
    {
        CloseHandle(thread);
        return true;
    }
    return false;
}

auto current_threads() -> enumerable<DWORD>
{
    const auto pid = GetCurrentProcessId();
    // for current process
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw std::system_error{gsl::narrow_cast<int>(GetLastError()),
                                std::system_category(),
                                "CreateToolhelp32Snapshot"};

    auto h = gsl::finally([=]() noexcept { CloseHandle(snapshot); });
    auto entry = THREADENTRY32{};
    entry.dwSize = sizeof(entry);
    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);
         entry.dwSize = sizeof(entry))
        // filter other process's threads
        if (entry.th32OwnerProcessID != pid)
            co_yield entry.th32ThreadID;
}
