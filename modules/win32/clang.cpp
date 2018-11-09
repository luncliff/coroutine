// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "../linkable.h"

#include <cstdint>

// We are using VC++ headers, but the compiler is not msvc.
// Redirect some intrinsics from msvc to clang
#ifndef __clang__
#error "This file must be compiled with clang 6.0 or later";
#else

// https://clang.llvm.org/docs/LanguageExtensions.html#c-coroutines-support-builtins
// https://llvm.org/docs/Coroutines.html#example
// void  __builtin_coro_resume(void *addr);
// void  __builtin_coro_destroy(void *addr);
// bool  __builtin_coro_done(void *addr);
// void *__builtin_coro_promise(void *addr, int alignment, bool from_promise)
// size_t __builtin_coro_size()
// void  *__builtin_coro_frame()
// void  *__builtin_coro_free(void *coro_frame)
//
// void  *__builtin_coro_id(int align, void *promise, void *fnaddr, void *parts)
// bool   __builtin_coro_alloc()
// void  *__builtin_coro_begin(void *memory)
// void   __builtin_coro_end(void *coro_frame, bool unwind)
// char   __builtin_coro_suspend(bool final)
// bool   __builtin_coro_param(void *original, void *copy)

extern "C" size_t _coro_resume(void* a) noexcept(false)
{
    struct _Resumable_frame_prefix
    {
        typedef void(__cdecl* _Resume_fn)(void*);
        _Resume_fn _Fn;
        uint16_t _Index;
        uint16_t _Flags;
    };
    auto* frame = reinterpret_cast<_Resumable_frame_prefix*>(a);

    __builtin_coro_resume(a);
    return 0;
}

extern "C" void _coro_destroy(void* a)
{
    return __builtin_coro_destroy(a); //
}

// extern "C" _INTERFACE_ //
//     size_t
//     _coro_done(void* a)
// {
//     const bool is_done = __builtin_coro_done(a);
//     return static_cast<size_t>(is_done);
// }
// __builtin_coro_promise

#endif
