// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
//
// ---------------------------------------------------------------------------
#include <coroutine/enumerable.hpp>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>
#include <thread/types.h>

#include <Windows.h>
#include <sdkddkver.h>

using namespace std;

extern thread_registry registry;

auto current_threads() noexcept(false) -> enumerable<thread_id_t>;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    UNREFERENCED_PARAMETER(instance);
    try
    {
        const auto tid = current_thread_id();
        if (tid == thread_id_t{0})
            throw runtime_error{"suspecious thread id"};

        if (reason == DLL_THREAD_ATTACH)
        {
            // add current thread
        }
        else if (reason == DLL_THREAD_DETACH)
        {
            // remove current thread
        }
        else if (reason == DLL_PROCESS_ATTACH)
        {
            // setup messaging for library
            // add existing threads
            // for (auto t: current_threads())
            //    *registry.reserve(static_cast<uint64_t>(t)) = nullptr;

            // add current thread
        }
        else if (reason == DLL_PROCESS_DETACH)
        {
            // remove current thread
            // remove existing threads
            // teardown messaging for library
        }
        return TRUE;
    }
    catch (const exception& e)
    {
        ::perror(e.what());
    }
    return FALSE;
}

#include <tlhelp32.h>

auto current_threads() noexcept(false) -> enumerable<thread_id_t>
{
    auto pid = GetCurrentProcessId();
    // for current process
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw system_error{static_cast<int>(GetLastError()), system_category(),
                           "CreateToolhelp32Snapshot"};

    // auto h = gsl::finally([=]() { CloseHandle(snapshot); });
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

    CloseHandle(snapshot);
}

extern thread_local thread_data current_data;

// Prevent race with APC callback
void CALLBACK deliver(const message_t msg) noexcept(false)
{
    static_assert(sizeof(message_t) == sizeof(ULONG_PTR));

    if (current_data.queue.push(msg) == false)
        throw runtime_error{"failed to deliver message"};
}

void lazy_delivery(thread_id_t thread_id, message_t msg) noexcept(false)
{
    SleepEx(0, true); // handle some APC

    DWORD ec{}, tid = static_cast<DWORD>(thread_id);
    // receiver thread
    HANDLE thread = OpenThread(THREAD_SET_CONTEXT, FALSE, tid);
    if (thread == NULL)
    {
        ec = GetLastError();
        throw system_error{static_cast<int>(ec), system_category(),
                           "OpenThread"};
    }

    // use apc queue to deliver message
    if (QueueUserAPC(reinterpret_cast<PAPCFUNC>(deliver), thread, msg.u64))
        // success. close handle and return
        CloseHandle(thread);
    else
    {
        ec = GetLastError(); 
        CloseHandle(thread);
        throw system_error{static_cast<int>(ec), system_category(),
                           "QueueUserAPC"};
    }
}
