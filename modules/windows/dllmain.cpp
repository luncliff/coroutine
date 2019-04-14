// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <coroutine/return.h>
#include <coroutine/yield.hpp>

#include <gsl/gsl_util>

// clang-format off
#include <Windows.h>
#include <sdkddkver.h>
#include <TlHelp32.h>
// clang-format on

using namespace std;

auto current_threads() -> coro::enumerable<DWORD>
{
    const auto pid = GetCurrentProcessId();
    // for current process
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "CreateToolhelp32Snapshot"};

    auto h = gsl::finally([=]() noexcept { CloseHandle(snapshot); });

    auto entry = THREADENTRY32{};
    entry.dwSize = sizeof(entry);

    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);
         entry.dwSize = sizeof(entry))
    {
        // filter other process's threads
        if (entry.th32OwnerProcessID != pid)
            co_yield entry.th32ThreadID;
    }
}

std::array<char, NI_MAXHOST> hnbuf{};
