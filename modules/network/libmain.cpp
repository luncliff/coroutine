
#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#define PROLOGUE
#define EPILOGUE
#else
#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))
#endif

#ifdef _WIN32
#include <Windows.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        return TRUE;
    return TRUE;
}

#endif
