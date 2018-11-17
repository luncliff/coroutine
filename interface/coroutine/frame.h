// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Note
//      Header to support coroutine frame difference between compilers
//      This file will focus on compiler intrinsics and
//      follow semantics of msvc intrinsics in `coroutine_handle<>`
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_FRAME_PREFIX_HPP
#define COROUTINE_FRAME_PREFIX_HPP

// ---- ---- compiler check for constexpr if ---- ----

#if _MSC_VER // <-- need alternative
             //     since it might be declared explicitly
static constexpr auto is_msvc = true;
#else
static constexpr auto is_msvc = false;
#endif // _MSC_VER
#if __clang__
static constexpr auto is_clang = true;
#else
static constexpr auto is_clang = false;
#endif // __clang__
#if __GNUC__
static constexpr auto is_gcc = true;
#else
static constexpr auto is_gcc = false;
#endif // __GNUC__

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

#include <experimental/coroutine>
#include <type_traits>

template<typename T>
constexpr auto aligned_size_v = ((sizeof(T) + 16 - 1) & ~(16 - 1));

using procedure_t = void(__cdecl*)(void*); // ! goto !

// - Note
//      MSVC coroutine frame's prefix. See `_Resumable_frame_prefix`
//
//      | Promise(?) | Frame Prefix(16) | Local variables(?) |
//
struct msvc_frame_prefix final
{
    procedure_t factivate;
    uint16_t index;
    uint16_t flag;
};
static_assert(sizeof(msvc_frame_prefix) == 16);

// - Note
//      Clang coroutine frame's prefix. See reference docs above
//
//      | Frame Prefix(16) | Promise(?) | ? | Local variables(?) |
//
struct clang_frame_prefix final
{
    procedure_t factivate;
    procedure_t fdestroy;
};
static_assert(sizeof(clang_frame_prefix) == 16);

// ---- MSVC Compiler intrinsic ----

// extern "C" size_t _coro_resume(void*);
// extern "C" void _coro_destroy(void*);
// extern "C" size_t _coro_done(void*);
// extern "C" size_t _coro_frame_size();
// extern "C" void* _coro_frame_ptr();
// extern "C" void _coro_init_block();
// extern "C" void* _coro_resume_addr();
// extern "C" void _coro_init_frame(void*);
// extern "C" void _coro_save(size_t);
// extern "C" void _coro_suspend(size_t);
// extern "C" void _coro_cancel();
// extern "C" void _coro_resume_block();

#ifdef __clang__

//
//  Note
//      VC++ header expects msvc intrinsics. Redirect them to Clang intrinsics.
//      If the project uses libc++ header files, this code won't be a problem
//      because they wont't be used
//  Reference
//      https://clang.llvm.org/docs/LanguageExtensions.html#c-coroutines-support-builtins
//      https://llvm.org/docs/Coroutines.html#example
//

#include <array>
#include <utility>

// void  __builtin_coro_resume(void *addr);
inline size_t _coro_resume(void* addr)
{
    // auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    __builtin_coro_resume(addr);

    // see `coroutine_handle<void>` in VC++ header
    //
    // There was no way but to place this intrinsic here because
    // `coroutine_handle<void>` doesn't use intrinsic to check
    // it's coroutine is done
    return _coro_done(addr);
}

// bool  __builtin_coro_done(void *addr);
inline size_t _coro_done(void* addr)
{
    // expect: coroutine == suspended
    // expect: coroutine != destroyed

    auto* m = reinterpret_cast<msvc_frame_prefix*>(addr);
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);

    // what if resume just bypass and destroy evenything?
    // see `coroutine_handle<void>` in VC++ header
    if (__builtin_coro_done(c))
    {
        // expect: c->activate == nullptr

        // If the coroutine is finished, it's ok to override the value
        //  in the frame since they won't be visible anyway.
        // Since VC++ implementation doesn't rely on intrinsic,
        //  we have to mark the frame's index manually

        // m->index = 0; // <-- like this

        // However, `fdestroy` is modified after `m->index = 0;`
        //  So we will swap it with `factivate` and
        //  make it work like `m->index = 0;`
        std::swap(c->factivate, c->fdestroy);

        // ensure: m->index == 0);
    }
    return m->index;
}

// void  __builtin_coro_destroy(void *addr);
inline void _coro_destroy(void* addr)
{
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    // Notice the trick we used in `_coro_done`.
    // since we exchanged the function pointers, we need to revoke the change
    // before start intrinsic;
    std::swap(c->factivate, c->fdestroy);
    __builtin_coro_destroy(c);
}

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

#endif // __clang__

#endif // COROUTINE_FRAME_PREFIX_HPP