// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Note
//      Header to support coroutine frame difference between compilers
//      This file will focus on compiler intrinsics and
//      follow semantics of msvc intrinsics in `coroutine_handle<>`
//  Reference
//      https://wg21.link/p0057
//      <experimental/resumable> from Microsoft Corperation
//      <experimental/coroutine> from LLVM libcxx 6.0.0+
//      http://clang.llvm.org/docs/LanguageExtensions.html#c-coroutines-support-builtins
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_FRAME_PREFIX_HPP
#define COROUTINE_FRAME_PREFIX_HPP

#include <cassert>
#include <cstdint>
#include <type_traits>

#if _MSC_VER // <-- need alternative
             //     since it might be declared explicitly
static constexpr auto is_msvc = true;
static constexpr auto is_clang = false;
static constexpr auto is_gcc = false;

#elif __clang__

static constexpr auto is_msvc = false;
static constexpr auto is_clang = true;
static constexpr auto is_gcc = false;

#else // __GNUC__ is missing
#error "compier doesn't support coroutine"
#endif

template<typename T>
constexpr auto aligned_size_v = ((sizeof(T) + 16 - 1) & ~(16 - 1));

using procedure_t = void(__cdecl*)(void*);

// - Note
//      MSVC coroutine frame's prefix
//      Reference <experimental/resumable> for the detail
// - Layout
//      +------------+------------------+--------------------+
//      | Promise(?) | Frame Prefix(16) | Function Locals(?) |
//      +------------+------------------+--------------------+
struct msvc_frame_prefix final
{
    procedure_t factivate;
    uint16_t index;
    uint16_t flag;
};
static_assert(sizeof(msvc_frame_prefix) == 16);

// - Note
//      Clang coroutine frame's prefix
// - Layout
//      +------------------+------------+---+--------------------+
//      | Frame Prefix(16) | Promise(?) | ? | Local variables(?) |
//      +------------------+------------+---+--------------------+
struct clang_frame_prefix final
{
    procedure_t factivate;
    procedure_t fdestroy;
};
static_assert(sizeof(clang_frame_prefix) == 16);

// redirect for `_coro_done`
bool _coro_finished(msvc_frame_prefix*) noexcept;

// -------------------- Compiler intrinsic: MSVC  --------------------

extern "C" size_t _coro_resume(void*);
extern "C" void _coro_destroy(void*);
extern "C" size_t _coro_done(void*); // <- leads compiler error
// extern "C" size_t _coro_frame_size();
// extern "C" void* _coro_frame_ptr();
// extern "C" void _coro_init_block();
// extern "C" void* _coro_resume_addr();
// extern "C" void _coro_init_frame(void*);
// extern "C" void _coro_save(size_t);
// extern "C" void _coro_suspend(size_t);
// extern "C" void _coro_cancel();
// extern "C" void _coro_resume_block();

// -------------------- Compiler intrinsic: Clang --------------------

extern "C" bool __builtin_coro_done(void*);
extern "C" void __builtin_coro_resume(void*);
extern "C" void __builtin_coro_destroy(void*);
// void *__builtin_coro_promise(void *addr, int alignment, bool from_promise)
// size_t __builtin_coro_size()
// void  *__builtin_coro_frame()
// void  *__builtin_coro_free(void *coro_frame)
// void  *__builtin_coro_id(int align, void *promise, void *fnaddr, void *parts)
// bool   __builtin_coro_alloc()
// void  *__builtin_coro_begin(void *memory)
// void   __builtin_coro_end(void *coro_frame, bool unwind)
// char   __builtin_coro_suspend(bool final)
// bool   __builtin_coro_param(void *original, void *copy)

namespace std::experimental
{
// traits to enforce promise_type
template<typename ReturnType, typename... Args>
struct coroutine_traits
{
    using promise_type = typename ReturnType::promise_type;
};

template<typename PromiseType = void>
class coroutine_handle;

template<>
class coroutine_handle<void>
{
  protected:
    // The purpose of this type is
    //  to provide more type information
    //  and prepare for future adaptation. (especially gcc)
    union prefix_t {
        void* v{};
        msvc_frame_prefix* m;
        clang_frame_prefix* c;
        // gcc_frame_prefix* g;
    };

    prefix_t prefix;

    static_assert(sizeof(prefix_t) == sizeof(void*));

  public:
    coroutine_handle() noexcept : prefix{nullptr} {}
    coroutine_handle(std::nullptr_t) noexcept : prefix{nullptr} {}

    coroutine_handle& operator=(nullptr_t) noexcept
    {
        prefix.v = nullptr;
        return *this;
    }

    operator bool() const noexcept { return prefix.v != nullptr; }

    void resume() noexcept(false)
    {
        if constexpr (is_msvc)
        {
            _coro_resume(prefix.m);
        }
        else if constexpr (is_clang)
        {
            __builtin_coro_resume(prefix.c);
        }
    }

    void destroy() noexcept(false)
    {
        if constexpr (is_msvc)
        {
            _coro_destroy(prefix.m);
        }
        else if constexpr (is_clang)
        {
            __builtin_coro_destroy(prefix.c);
        }
    }

    bool done() const noexcept
    {
        if constexpr (is_msvc)
        {
            return _coro_finished(prefix.m);
        }
        else if constexpr (is_clang)
        {
            return __builtin_coro_done(prefix.c);
        }
        else if constexpr (is_gcc)
        {
            return false;
        }
    }

  public:
    constexpr void* address() const noexcept { return prefix.v; }

    static coroutine_handle from_address(void* addr)
    {
        coroutine_handle rh{};
        rh.prefix.v = addr;
        return rh;
    }
};

template<typename PromiseType>
class coroutine_handle : public coroutine_handle<void>
{
    using base_type = coroutine_handle<void>;

  public:
    using promise_type = PromiseType;

  private:
    static promise_type* from_frame(prefix_t addr) noexcept
    {
        if constexpr (is_clang)
        {
            // calculate the location of the coroutine frame prefix
            auto prefix = addr.c;
            // for clang, promise is placed just after frame prefix
            auto* promise = reinterpret_cast<promise_type*>(prefix + 1);
            return promise;
        }
        else if constexpr (is_msvc)
        {
            auto ptr = reinterpret_cast<char*>(addr.m);
            // for msvc, promise is placed before frame prefix
            auto* promise = reinterpret_cast<promise_type*>(
                ptr - aligned_size_v<promise_type>);
            return promise;
        }
        if constexpr (is_gcc)
        {
            // !!! crash !!!
            return nullptr;
        }
    }

  public:
    using coroutine_handle<void>::coroutine_handle;

    coroutine_handle& operator=(nullptr_t)
    {
        base_type::operator=(nullptr);
        return *this;
    }

    auto promise() const -> const promise_type&
    {
        promise_type* p = from_frame(prefix);
        return *p;
    }
    auto promise() -> promise_type&
    {
        promise_type* p = from_frame(prefix);
        return *p;
    }

  public:
    static coroutine_handle from_address(void* addr)
    {
        coroutine_handle rh{};
        rh.prefix.v = addr;
        return rh;
    }

    static coroutine_handle from_promise(promise_type& prom)
    {
        promise_type* promise = std::addressof(prom);

        // calculate the location of the coroutine frame prefix
        if constexpr (is_clang)
        {
            void* prefix =
                reinterpret_cast<char*>(promise) - sizeof(clang_frame_prefix);
            return coroutine_handle::from_address(prefix);
        }
        else if constexpr (is_msvc)
        {
            void* prefix =
                reinterpret_cast<char*>(promise) + aligned_size_v<promise_type>;
            return coroutine_handle::from_address(prefix);
        }
        return coroutine_handle{};
    }
};

inline bool operator==(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept
{
    return lhs.address() == rhs.address();
}
inline bool operator!=(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept
{
    return !(lhs == rhs);
}
inline bool operator<(const coroutine_handle<void> lhs,
                      const coroutine_handle<void> rhs) noexcept
{
    return lhs.address() < rhs.address();
}
inline bool operator>(const coroutine_handle<void> lhs,
                      const coroutine_handle<void> rhs) noexcept
{
    return !(lhs < rhs);
}
inline bool operator<=(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept
{
    return !(lhs > rhs);
}
inline bool operator>=(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept
{
    return !(lhs < rhs);
}

struct suspend_never
{
    constexpr bool await_ready() const { return true; }
    void await_suspend(coroutine_handle<void>) const {}
    constexpr void await_resume() const {}
};

struct suspend_always
{
    constexpr bool await_ready() const { return false; }
    void await_suspend(coroutine_handle<void>) const {}
    constexpr void await_resume() const {}
};
} // namespace std::experimental

#ifdef _MSC_VER
// ---------------------------------------------------------------------------
// Helper:  MSVC

namespace std::experimental
{
//  - Note
//      Helper traits for MSVC's coroutine compilation.
//      The original code is in <experimental/resumable>
template<typename _Ret, typename... _Ts>
struct _Resumable_helper_traits
{
    using _Traits = coroutine_traits<_Ret, _Ts...>;
    using _PromiseT = typename _Traits::promise_type;
    using _Handle_type = coroutine_handle<_PromiseT>;

    static _PromiseT* _Promise_from_frame(void* addr) noexcept
    {
        auto& prom = _Handle_type::from_address(addr).promise();
        return std::addressof(prom);
    }

    static _Handle_type _Handle_from_frame(void* _Addr) noexcept
    {
        auto* p = _Promise_from_frame(_Addr);
        return _Handle_type::from_promise(*p);
    }

    static void _Set_exception(void* addr)
    {
        auto& prom = _Handle_type::from_address(addr).promise();
        prom->set_exception(std::current_exception());
    }

    static void _ConstructPromise(void* addr, void* func, int _HeapElision)
    {
        auto prefix = static_cast<msvc_frame_prefix*>(addr);
        prefix->factivate = static_cast<procedure_t>(func);

        uint32_t* ptr = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uintptr_t>(prefix) + sizeof(void*));
        *ptr = 2 + (_HeapElision ? 0 : 0x10000);

        auto* prom = _Promise_from_frame(prefix);
        ::new (prom) _PromiseT();
    }

    static void _DestructPromise(void* addr)
    {
        auto prefix = static_cast<msvc_frame_prefix*>(addr);
        auto* prom = _Promise_from_frame(prefix);
        prom->~_PromiseT();
    }
};

} // namespace std::experimental

// alternative of `_coro_done` of msvc for this library,
// it's renamed to avoid redefinition
inline bool _coro_finished(msvc_frame_prefix* prefix) noexcept
{
    // expect: coroutine == suspended
    // expect: coroutine != destroyed
    return prefix->index == 0;
}
#endif // _MSC_VER

#ifdef __clang__
// ---------------------------------------------------------------------------
// Helper:  clang-cl

//
//  Note
//      VC++ header expects msvc intrinsics. Redirect them to Clang intrinsics.
//      If the project uses libc++ header files, this code won't be a problem
//      because they wont't be used
//  Reference
//      https://clang.llvm.org/docs/LanguageExtensions.html#c-coroutines-support-builtins
//      https://llvm.org/docs/Coroutines.html#example
//

/*
#include <utility>

// bool  __builtin_coro_done(void *addr);
inline bool _coro_finished(msvc_frame_prefix* m) noexcept // _coro_done
{
    // expect: coroutine == suspended
    // expect: coroutine != destroyed
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);

    return __builtin_coro_done(c) ? 1 : 0;

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

        // ensure: m->index == 0;
    }
    return m->index == 0;
}

// void  __builtin_coro_resume(void *addr);
inline size_t _coro_resume(void* addr)
{
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    // auto* m = reinterpret_cast<msvc_frame_prefix*>(addr);

    auto fn = c->factivate;
    __builtin_coro_resume(c);

    //
    // If some coroutines doen't 'final_suspend',
    //  it's frame will be deleted after above resume operation.
    //
    if (c->factivate != nullptr && c->factivate != fn)
        // nullptr if the coroutine is 'final_suspend'ed
        // address mismatch because of free operation
        //
        // !!! But accessing freed address space yields access violation !!!
        //
        return 0;
    //
    // For 'final_suspend'ing coroutines,
    //  the following line is required to work with VC++ implementation

    //
    // See `coroutine_handle<void>` in VC++ header.
    // It doesn't rely on `_coro_done` to check its coroutine is returned
    //
    // Therefore, there was no way but to place
    //  additional intrinsic here...
    return _coro_done(addr);
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
*/
#endif // __clang__

#endif // COROUTINE_FRAME_PREFIX_HPP