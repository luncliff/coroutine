// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/enumerable.hpp>
// #include <gsl/gsl_util>

#include <Windows.h> // System API
#include <tlhelp32.h>

auto current_threads() -> enumerable<DWORD>
{
    auto pid = GetCurrentProcessId();
    // for current process
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw std::system_error{static_cast<int>(GetLastError()),
                                std::system_category(),
                                "CreateToolhelp32Snapshot"};

    // auto h = gsl::finally([=]() { CloseHandle(snapshot); });
    auto entry = THREADENTRY32{};
    entry.dwSize = sizeof(entry);

    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);
         entry.dwSize = sizeof(entry))
        // filter other process's threads
        if (entry.th32OwnerProcessID != pid)
            co_yield entry.th32ThreadID;

    CloseHandle(snapshot);
}
