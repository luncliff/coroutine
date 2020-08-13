/**
 * @brief   Check the compiler supports coroutine intrinsics
 * @author  github.com/luncliff (luncliff@gmail.com)
 * 
 * @see     LLVM libc++ <experimental/coroutine>
 * @see     https://llvm.org/docs/Coroutines.html
 * @see     Microsoft STL <coroutine>
 * @see     https://github.com/iains/gcc-cxx-coroutines
 */
#if defined(__has_builtin)

// known functions
static_assert(__has_builtin(__builtin_coro_done));
static_assert(__has_builtin(__builtin_coro_resume));
static_assert(__has_builtin(__builtin_coro_destroy));
static_assert(__has_builtin(__builtin_coro_promise));

#if defined(__clang__)
static_assert(__has_builtin(__builtin_coro_size));
static_assert(__has_builtin(__builtin_coro_frame));
static_assert(__has_builtin(__builtin_coro_free));
static_assert(__has_builtin(__builtin_coro_id));
static_assert(__has_builtin(__builtin_coro_begin));
static_assert(__has_builtin(__builtin_coro_end));
static_assert(__has_builtin(__builtin_coro_suspend));
static_assert(__has_builtin(__builtin_coro_param));
#endif

#else

/**
 * @brief   Try direct use of MSVC/Clang/GCC `__builtin_coro_*` intrinsics 
 *          (without declaration)
 * @note    For MSVC, if the test failes, we have to use `_coro_*` intrinsics
 * @see     <experimental/resumable>
 */
bool has_builtin = __builtin_coro_done(nullptr);

#endif
