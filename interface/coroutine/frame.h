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

template<typename Base>
struct attach_u64_type_size : public Base
{
    static_assert(std::alignment_of_v<Base> == 8 ||
                  std::alignment_of_v<Base> == 4);

  public:
    const uint64_t type_size;

  public:
    attach_u64_type_size() noexcept
        : Base{}, type_size{sizeof(Base) + sizeof(uint64_t)}
    {
    }
};

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

// - Note
//      Clang coroutine frame's prefix. See reference docs above
struct clang_frame final
{
    procedure_t factivate;
    procedure_t fdestroy;

    static auto from_msvc_address(void* addr) noexcept
    {
        uint64_t* ptr = reinterpret_cast<uint64_t*>(addr);
        constexpr auto prefix_size = sizeof(clang_frame);
        const auto promise_size = ptr[-1]; // reserved space for type's size
                                           // see `attach_u64_type_size`
        ptr = ptr - promise_size / sizeof(uint64_t);   // size of promise_type
        ptr = ptr - prefix_size / sizeof(procedure_t); // size of frame prefix
        return reinterpret_cast<clang_frame*>(ptr);
    }
};

// void  __builtin_coro_resume(void *addr);
inline size_t _coro_resume(void* addr)
{
    auto* frame = clang_frame::from_msvc_address(addr);
    __builtin_coro_resume(frame);
    return _coro_done(addr);
}

// void  __builtin_coro_destroy(void *addr);
inline void _coro_destroy(void* addr)
{
    void* frame = clang_frame::from_msvc_address(addr);
    return __builtin_coro_destroy(frame);
}

// bool  __builtin_coro_done(void *addr);
inline size_t _coro_done(void* addr)
{
    auto* mf = reinterpret_cast<msvc_frame*>(addr);
    auto* cf = clang_frame::from_msvc_address(addr);

    if (__builtin_coro_done(cf))
        // see `msvc_frame`
        mf->index = 0;

    return mf->index;
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