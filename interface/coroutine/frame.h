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

#ifndef LINKABLE_DLL_MACRO
#define LINKABLE_DLL_MACRO

#ifdef _MSC_VER // MSVC
#define _HIDDEN_
#ifdef _WINDLL
#define _INTERFACE_ __declspec(dllexport)
#else
#define _INTERFACE_ __declspec(dllimport)
#endif

#elif __GNUC__ || __clang__ // GCC or Clang
#define _INTERFACE_ __attribute__((visibility("default")))
#define _HIDDEN_ __attribute__((visibility("hidden")))

#else
#error "unexpected compiler"

#endif // compiler check
#endif // LINKABLE_DLL_MACRO

#ifndef COROUTINE_FRAME_PREFIX_HPP
#define COROUTINE_FRAME_PREFIX_HPP

#include <experimental/coroutine>
#include <type_traits>

using procedure_t = void(__cdecl*)(void*);

// - Note
//      MSVC coroutine frame's prefix. See `_Resumable_frame_prefix`
struct msvc_frame final
{
    procedure_t factivate;
    uint16_t index;
    uint16_t flag;
};
static_assert(sizeof(msvc_frame) == 16);
// static constexpr auto _ALIGN_REQ = 16;
// static constexpr auto exp_prom_size = 17;
// static constexpr auto prom_align =
//     (exp_prom_size + _ALIGN_REQ - 1) & ~(_ALIGN_REQ - 1);

// - Note
//      Clang coroutine frame's prefix. See reference docs above
struct clang_frame final
{
    procedure_t factivate;
    procedure_t fdestroy;
};
static_assert(sizeof(clang_frame) == 16);

_INTERFACE_ void dump_frame(void* frame) noexcept;

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
//      VC++ header expects msvc intrinsics. Redirect them to Clang intrinsics
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
    auto* m = reinterpret_cast<msvc_frame*>(addr);
    auto* c = reinterpret_cast<clang_frame*>(addr);

    std::printf("_coro_resume: %p\n", addr);
    // constexpr auto finished = 0;
    // auto status = _coro_done(addr);
    // if (status == finished) // if finished, index is 0;
    //     return m->index = 0;

    __builtin_coro_resume(c);
    return 1;
}

// void  __builtin_coro_destroy(void *addr);
inline void _coro_destroy(void* addr)
{
    // auto* m = reinterpret_cast<msvc_frame*>(addr);
    auto* c = reinterpret_cast<clang_frame*>(addr);

    std::printf("_coro_destroy: %p\n", addr);

    // dump_frame(c);
    return __builtin_coro_destroy(c);
}

// bool  __builtin_coro_done(void *addr);
inline size_t _coro_done(void* addr)
{
    auto* m = reinterpret_cast<msvc_frame*>(addr);
    auto* c = reinterpret_cast<clang_frame*>(addr);

    std::printf("_coro_done: %p\n", addr);
    if (__builtin_coro_done(c))
        // see `msvc_frame`.
        // if the coroutine is finished, it's ok to override the value
        //  in the frame since they won't be visible anyway
        m->index = 0;

    // dump_frame(c);
    return m->index;
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