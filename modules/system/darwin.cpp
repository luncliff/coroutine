/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief Check the compiler supports intrinsics for C++ Coroutines
 */

// expect the following headers are available
// clang-format off
#include <coroutine/frame.h>
#include <coroutine/unix.h>
#include <coroutine/return.h>
#include "internal/yield.hpp"
// clang-format on

#if defined(__clang__)
#else
// we must error for the compilation.
// however, this section will be left blank
// to allow possible a clang-compatible one
#endif

// in-use functions
static_assert(__has_builtin(__builtin_coro_done));
static_assert(__has_builtin(__builtin_coro_resume));
static_assert(__has_builtin(__builtin_coro_destroy));
static_assert(__has_builtin(__builtin_coro_promise));
// known functions
static_assert(__has_builtin(__builtin_coro_size));
static_assert(__has_builtin(__builtin_coro_frame));
static_assert(__has_builtin(__builtin_coro_free));
static_assert(__has_builtin(__builtin_coro_id));
static_assert(__has_builtin(__builtin_coro_begin));
static_assert(__has_builtin(__builtin_coro_end));
static_assert(__has_builtin(__builtin_coro_suspend));
static_assert(__has_builtin(__builtin_coro_param));
