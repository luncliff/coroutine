// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <stdexcept>

#ifdef _WIN32
#define PROCEDURE
#else
#define PROCEDURE __attribute__((constructor))
#endif

#ifdef _WIN32
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    UNREFERENCED_PARAMETER(instance);
    try
    {
        const auto tid = current_thread_id();
        if (tid == thread_id_t{0})
            throw std::runtime_error{"suspecious thread id"};

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
    catch (const std::exception& e)
    {
        ::perror(e.what());
    }
    return FALSE;
}

#endif // _WIN32
