// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Reference
//      -
//      https://docs.microsoft.com/en-us/windows/desktop/ProcThread/using-the-thread-pool-functions
//
// ---------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <CppUnitTest.h>
#include <Windows.h>
#include <sdkddkver.h>
#include <threadpoolapiset.h>

#include <array>

#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

using namespace std::experimental;

char buffer[256]{};

void report_thread_entry(DWORD thread_id, DWORD reason)
{
    sprintf_s(buffer, "thread %d reason %d", (int)thread_id, (int)reason);
    Logger::WriteMessage(buffer);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    report_thread_entry(GetCurrentThreadId(), reason);
    return TRUE;
}
