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

#include <cassert>

#include <Windows.h>
#include <sdkddkver.h>
#include <tlhelp32.h>

using namespace std;

DWORD tls_index = 0;

auto current_threads() noexcept(false) -> enumerable<thread_id_t>;
extern thread_registry registry;

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
            // auto tls_data = (LPVOID)LocalAlloc(LPTR, sizeof(thread_data));
            // if (tls_data)
            //{
            //    TlsSetValue(tls_index, tls_data);
            //    new (tls_data) thread_data{};
            //}
            auto ptr = get_local_data();
            if (ptr == nullptr)
                return FALSE;

            assert(*registry.search(static_cast<uint64_t>(current_thread_id()))
                   != nullptr);
        }
        else if (reason == DLL_THREAD_DETACH)
        {
            // remove current thread
            auto ptr = get_local_data();
            if (ptr)
            {
                ptr->~thread_data();
                LocalFree((HLOCAL)ptr);
            }
        }
        else if (reason == DLL_PROCESS_ATTACH)
        {
            // setup TLS
            tls_index = TlsAlloc();
            if (tls_index == TLS_OUT_OF_INDEXES)
                return FALSE;

            // add existing threads
            for (auto t : current_threads())
                *registry.reserve(static_cast<uint64_t>(t)) = nullptr;

            // add current thread
            auto ptr = get_local_data();
            if (ptr == nullptr)
                return FALSE;

            // add current thread
        }
        else if (reason == DLL_PROCESS_DETACH)
        {
            // remove current thread
            auto ptr = get_local_data();
            if (ptr)
            {
                ptr->~thread_data();
                LocalFree((HLOCAL)ptr);
            }

            // remove existing threads
            for (auto t : current_threads())
                registry.remove(static_cast<uint64_t>(t));

            // teardown tls
            TlsFree(tls_index);

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

thread_data* get_local_data() noexcept
{
    auto tls_data = TlsGetValue(tls_index);
    if (tls_data == nullptr)
    {
        tls_data = (LPVOID)LocalAlloc(LPTR, sizeof(thread_data));

        if (tls_data == nullptr || TlsSetValue(tls_index, tls_data) == FALSE)
            return nullptr;

        new (tls_data) thread_data{};
    }
    return static_cast<thread_data*>(tls_data);
}

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

// Prevent race with APC callback
void CALLBACK deliver(const message_t msg) noexcept(false)
{
    static_assert(sizeof(message_t) == sizeof(ULONG_PTR));

    auto* thread_data = get_local_data();
    assert(thread_data != nullptr);

    if (thread_data->queue.push(msg) == false)
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
    {
        CloseHandle(thread);
        SleepEx(0, true); // handle some APC
    }
    else
    {
        ec = GetLastError();
        CloseHandle(thread);
        throw system_error{static_cast<int>(ec), system_category(),
                           "QueueUserAPC"};
    }
}
