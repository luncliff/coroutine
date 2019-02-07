// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

// We are using VC++ headers, but the compiler is not msvc.
#ifndef __clang__
#error "This library must be compiled with clang 6.0 or later";
#endif

// manual symbol export for linking

// clang-format off
// __FUNCTION__ __FUNCDNAME__
#define LINKER_EXPORT_1(name, dname) "/export:"#name"="#dname

// #pragma message(LINKER_EXPORT_1(__FUNCTION__, __FUNCDNAME__))
// #pragma comment(linker, LINKER_EXPORT_1(__FUNCTION__, __FUNCDNAME__))

// clang-format on
