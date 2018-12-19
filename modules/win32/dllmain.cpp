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
#include <thread/types.h>

#include <cassert>

#include <Windows.h>
#include <sdkddkver.h>

#include <TlHelp32.h>

using namespace std;

DWORD tls_index = 0;

auto current_threads() noexcept(false) -> enumerable<thread_id_t>;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    UNREFERENCED_PARAMETER(instance);

    auto* registry = get_thread_registry();
    switch (reason)
    {
    case DLL_PROCESS_DETACH:
        // remove existing threads
        for (auto tid : current_threads())
            registry->erase(tid);

        // teardown tls
        TlsFree(tls_index);
        break;
    case DLL_PROCESS_ATTACH:
        // setup TLS
        tls_index = TlsAlloc();
        if (tls_index == TLS_OUT_OF_INDEXES)
            return FALSE;

        // add existing threads
        for (auto tid : current_threads())
            *(registry->find_or_insert(tid)) = nullptr;

    case DLL_THREAD_ATTACH:
        // add current thread
        if (get_local_data() == nullptr)
            return FALSE;

        break;
    case DLL_THREAD_DETACH:
        // remove current thread
        if (auto tls = get_local_data())
        {
            tls->~thread_data();
            LocalFree((HLOCAL)tls);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

thread_data* get_local_data() noexcept
{
    if (auto tls = TlsGetValue(tls_index))
        return static_cast<thread_data*>(tls);

    auto tls = (LPVOID)LocalAlloc(LPTR, sizeof(thread_data));

    // allocation / remember failure
    if (tls == nullptr || TlsSetValue(tls_index, tls) == FALSE)
        return nullptr;

    return new (tls) thread_data{};
}

bool check_thread_exists(thread_id_t id) noexcept
{
    // if receiver thread exists, OpenThread will be successful
    HANDLE thread = OpenThread(
        // for current implementation,
        // lazy_delivery requires this operation
        THREAD_SET_CONTEXT,
        FALSE,
        static_cast<DWORD>(id));

    if (thread == NULL) // *possibly* invalid thread id
        return false;

    CloseHandle(thread);
    return true;
}

auto current_threads() noexcept(false) -> enumerable<thread_id_t>
{
    auto pid = GetCurrentProcessId();
    // for current process
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw system_error{static_cast<int>(GetLastError()),
                           system_category(),
                           "CreateToolhelp32Snapshot"};

    auto entry = THREADENTRY32{};
    entry.dwSize = sizeof(entry);

    thread_id_t tid{};
    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);
         entry.dwSize = sizeof(entry))
        // filter other process's threads
        if (entry.th32OwnerProcessID == pid)
        {
            tid = static_cast<thread_id_t>(entry.th32ThreadID);
            co_yield tid;
        }

    // the line should be replaced to gsl::finally
    CloseHandle(snapshot);
}

// Prevent race with APC callback
void CALLBACK deliver(const message_t msg) noexcept(false)
{
    static_assert(sizeof(message_t) == sizeof(ULONG_PTR));

    auto* tls = get_local_data();
    assert(tls != nullptr);

    if (tls->post(msg) == false)
        throw runtime_error{"failed to deliver message"};
}

void lazy_delivery(thread_id_t thread_id, message_t msg) noexcept(false)
{
    // receiver thread
    HANDLE thread
        = OpenThread(THREAD_SET_CONTEXT, FALSE, static_cast<DWORD>(thread_id));
    if (thread == NULL)
        throw system_error{
            static_cast<int>(GetLastError()), system_category(), "OpenThread"};

    // use apc queue to deliver message
    if (QueueUserAPC(reinterpret_cast<PAPCFUNC>(deliver), thread, msg.u64))
        CloseHandle(thread);
    else
    {
        const auto ec = GetLastError();
        CloseHandle(thread);
        throw system_error{
            static_cast<int>(ec), system_category(), "QueueUserAPC"};
    }
}
