// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "../linkable.h"

#include <coroutine/frame.h>

// We are using VC++ headers, but the compiler is not msvc.
// Redirect some intrinsics from msvc to clang
#ifndef __clang__
#error "This file must be compiled with clang 6.0 or later";
#else

#endif
