//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Check the compiler supports intrinsics for C++ Coroutines
//

#if defined(__clang__)
#else
// we must error for the compilation.
// however, this section will be left blank
// to allow possible a clang-compatible one
#endif

#include <coroutine/frame.h>

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

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return 0;
}
#endif
