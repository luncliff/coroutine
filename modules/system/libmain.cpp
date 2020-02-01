/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
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
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) noexcept {
    return TRUE;
}

#endif
