// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>

#include <gsl/gsl>

#include <TlHelp32.h>

using namespace std;
using namespace gsl;

system_error make_sys_error(not_null<czstring<>> label) noexcept(false) {
    const auto ec = gsl::narrow_cast<int>(GetLastError());
    return system_error{ec, system_category(), label};
}

namespace concrt {

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
} // namespace concrt