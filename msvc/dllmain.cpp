// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      This file is distributed under Creative Commons 4.0-BY License
//
// ---------------------------------------------------------------------------
#include <magic/date_time.hpp>

#include <cassert>

#ifdef _WIN32
#define PROCEDURE
#else
#define PROCEDURE __attribute__((constructor))
#endif

namespace magic
{
PROCEDURE void on_load(void *) noexcept(false)
{
    const magic::date_time now{};
    const auto calender = static_cast<std::tm>(now);
    assert(now.render().size() > 0);
}
} // namespace magic

#ifdef _WIN32

#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    try
    {
        if (fdwReason == DLL_PROCESS_ATTACH)
            magic::on_load(hinstDLL);

        return TRUE;
    }
    catch (std::exception &e)
    {
        ::perror(e.what());
        return FALSE;
    }
}

#endif // _WIN32
