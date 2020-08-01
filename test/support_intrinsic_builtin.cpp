/**
 * @brief   Check the compiler supports coroutine intrinsics
 * @see     libc++ <experimental/coroutine>
 */
#if 0
// defined(__has_builtin)

static_assert(__has_builtin(__builtin_coro_done));
static_assert(__has_builtin(__builtin_coro_resume));
static_assert(__has_builtin(__builtin_coro_destroy));
static_assert(__has_builtin(__builtin_coro_promise));
static_assert(__has_builtin(__builtin_coro_size));
static_assert(__has_builtin(__builtin_coro_frame));
// static_assert(__has_builtin(__builtin_coro_free));
// static_assert(__has_builtin(__builtin_coro_id));
// static_assert(__has_builtin(__builtin_coro_begin));
// static_assert(__has_builtin(__builtin_coro_end));
// static_assert(__has_builtin(__builtin_coro_suspend));
// static_assert(__has_builtin(__builtin_coro_param));

#else

/**
 * @brief   Try direct use of MSVC/Clang/GCC `__builtin_coro_*` intrinsics 
 *          (without declaration)
 * @note    For MSVC, if the test failes, we have to use `_coro_*` intrinsics
 * @see     <experimental/resumable>
 */
bool has_builtin = __builtin_coro_done(nullptr);

#endif
