// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

#ifdef _WIN32
#define PROCEDURE
#else
#define PROCEDURE __attribute__((constructor))
#endif

#include <array>
#include <cassert>
#include <numeric>

#include <coroutine/enumerable.hpp>
#include <coroutine/frame.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

extern void setup_messaging() noexcept(false);
extern void teardown_messaging() noexcept(false);
extern void add_messaging_thread(thread_id_t tid) noexcept(false);
extern void remove_messaging_thread(thread_id_t tid) noexcept(false);

#ifdef _WIN32
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

thread_id_t current_thread_id() noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    UNREFERENCED_PARAMETER(instance);

    const auto tid = current_thread_id();
    try
    {
        if (reason == DLL_THREAD_ATTACH)
        {
            add_messaging_thread(tid);
        }
        if (reason == DLL_THREAD_DETACH)
        {
            remove_messaging_thread(tid);
        }
        if (reason == DLL_PROCESS_ATTACH)
        {
            setup_messaging();
            add_messaging_thread(tid);
        }
        if (reason == DLL_PROCESS_DETACH)
        {
            remove_messaging_thread(tid);
            teardown_messaging();
        }
        return TRUE;
    }
    catch (const std::exception& e)
    {
        ::perror(e.what());
    }
    return FALSE;
}

#endif // _WIN32
