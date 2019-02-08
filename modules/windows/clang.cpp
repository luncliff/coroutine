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

// let's check the compiler supports coroutine functions

// in-use functions
static_assert(__has_builtin(__builtin_coro_done));
static_assert(__has_builtin(__builtin_coro_resume));
static_assert(__has_builtin(__builtin_coro_destroy));

// known functions
static_assert(__has_builtin(__builtin_coro_promise));
static_assert(__has_builtin(__builtin_coro_size));
static_assert(__has_builtin(__builtin_coro_frame));
static_assert(__has_builtin(__builtin_coro_free));
static_assert(__has_builtin(__builtin_coro_id));
static_assert(__has_builtin(__builtin_coro_begin));
static_assert(__has_builtin(__builtin_coro_end));
static_assert(__has_builtin(__builtin_coro_suspend));
static_assert(__has_builtin(__builtin_coro_param));
