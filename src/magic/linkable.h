// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Note
//      Macro helper for DLL interface declaration
//
// ---------------------------------------------------------------------------
#ifndef _LINKABLE_DLL_MACRO_
#define _LINKABLE_DLL_MACRO_

#ifdef _WIN32
#define _HIDDEN_
#ifdef _WINDLL
#define _INTERFACE_ __declspec(dllexport)
#define _C_INTERFACE_ extern "C" __declspec(dllexport)
#else
#define _INTERFACE_ __declspec(dllimport)
#define _C_INTERFACE_ extern "C" __declspec(dllimport)
#endif

#elif __APPLE__ || __linux__ || __unix__
#define _INTERFACE_ extern "C" __attribute__((visibility("default")))
#define _C_INTERFACE_ extern "C" __attribute__((visibility("default")))
#define _HIDDEN_ __attribute__((visibility("hidden")))

#else
#error "Unknown platform"
#endif

#endif // _LINKABLE_DLL_MACRO_
