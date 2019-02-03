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
#pragma once

#ifndef COROUTINE_FRAME_PREFIX_HPP
#define COROUTINE_FRAME_PREFIX_HPP
#pragma warning(push, 4)
#pragma warning(disable : 4455 4494 4577 4619 4643 4702 4984 4988)
#pragma warning(disable : 26490 26481 26476)

#include <cstdint>
#include <type_traits>

#if defined(__clang__)
static constexpr auto is_msvc = false;
static constexpr auto is_clang = true;
static constexpr auto is_gcc = false;

#elif defined(_MSC_VER)
static constexpr auto is_msvc = true;
static constexpr auto is_clang = false;
static constexpr auto is_gcc = false;

#else // __GNUC__ is missing
#error "compier doesn't support coroutine"
#endif

template <typename T>
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
static_assert(aligned_size_v<msvc_frame_prefix> == 16);

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
static_assert(aligned_size_v<clang_frame_prefix> == 16);

// - Note
//      Alternative of `_coro_done` of msvc for this library.
//      It is renamed to avoid redefinition
bool _coro_finished(const msvc_frame_prefix*) noexcept;

// -------------------- Compiler intrinsic: MSVC  --------------------

extern "C" size_t _coro_resume(void*);
extern "C" void _coro_destroy(void*);
extern "C" size_t _coro_done(void*); // <- leads compiler error

// -------------------- Compiler intrinsic: Clang --------------------

extern "C" bool __builtin_coro_done(void*);
extern "C" void __builtin_coro_resume(void*);
extern "C" void __builtin_coro_destroy(void*);

namespace std::experimental
{
// traits to enforce promise_type
template <typename ReturnType, typename... Args>
struct coroutine_traits
{
    using promise_type = typename ReturnType::promise_type;
};

template <typename PromiseType = void>
class coroutine_handle;

template <>
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
    };

    prefix_t prefix;

    static_assert(sizeof(prefix_t) == sizeof(void*));

  public:
    coroutine_handle() noexcept = default;
    ~coroutine_handle() noexcept = default;

    explicit coroutine_handle(std::nullptr_t) noexcept : prefix{nullptr}
    {
    }
    coroutine_handle(coroutine_handle const&) noexcept = default;
    coroutine_handle& operator=(coroutine_handle const&) noexcept = default;

    coroutine_handle(coroutine_handle&& rhs) noexcept : prefix{rhs.prefix}
    {
        rhs.prefix = prefix_t{};
    }
    coroutine_handle& operator=(coroutine_handle&& rhs) noexcept
    {
        std::swap(this->prefix.v, rhs.prefix.v);
        return *this;
    }

    coroutine_handle& operator=(nullptr_t) noexcept
    {
        prefix.v = nullptr;
        return *this;
    }

    operator bool() const noexcept
    {
        return prefix.v != nullptr;
    }

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

    void destroy() noexcept
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
    constexpr void* address() const noexcept
    {
        return prefix.v;
    }

    static coroutine_handle from_address(void* addr) noexcept
    {
        coroutine_handle rh{};
        rh.prefix.v = addr;
        return rh;
    }
};

template <typename PromiseType>
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
            auto* prefix = addr.c;
            // for clang, promise is placed just after frame prefix
            auto* promise = reinterpret_cast<promise_type*>(prefix + 1);
            return promise;
        }
        else if constexpr (is_msvc)
        {
            auto* ptr = reinterpret_cast<char*>(addr.m);
            // for msvc, promise is placed before frame prefix
            auto* promise = reinterpret_cast<promise_type*>(
                ptr - aligned_size_v<promise_type>);
            return promise;
        }
        else if constexpr (is_gcc)
        {
            // !!! crash !!!
            return nullptr;
        }
    }

  public:
    using coroutine_handle<void>::coroutine_handle;

    coroutine_handle& operator=(nullptr_t) noexcept
    {
        base_type::operator=(nullptr);
        return *this;
    }

    auto promise() const noexcept -> const promise_type&
    {
        promise_type* p = from_frame(prefix);
        return *p;
    }
    auto promise() noexcept -> promise_type&
    {
        promise_type* p = from_frame(prefix);
        return *p;
    }

  public:
    static coroutine_handle from_address(void* addr) noexcept
    {
        coroutine_handle rh{};
        rh.prefix.v = addr;
        return rh;
    }

    static coroutine_handle from_promise(promise_type& prom) noexcept
    {
        promise_type* promise = std::addressof(prom);

        // calculate the location of the coroutine frame prefix
        if constexpr (is_clang)
        {
            void* prefix
                = reinterpret_cast<char*>(promise) - sizeof(clang_frame_prefix);
            return coroutine_handle::from_address(prefix);
        }
        else if constexpr (is_msvc)
        {
            void* prefix = reinterpret_cast<char*>(promise)
                           + aligned_size_v<promise_type>;
            return coroutine_handle::from_address(prefix);
        }
        return coroutine_handle{};
    }
};
static_assert(sizeof(coroutine_handle<void>) == sizeof(void*));

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

class suspend_never
{
  public:
    constexpr bool await_ready() const noexcept
    {
        return true;
    }
    void await_suspend(coroutine_handle<void>) const noexcept
    {
        // Since the class won't suspend,
        // this function won't be invoked
    }
    constexpr void await_resume() const noexcept
    {
        // Nothing to do for this utility class
    }
};

class suspend_always
{
  public:
    constexpr bool await_ready() const noexcept
    {
        return false;
    }
    void await_suspend(coroutine_handle<void>) const noexcept
    {
        // The routine will suspend but the class ignores
        // resumable handle
    }
    constexpr void await_resume() const noexcept
    {
        // Nothing to do for this utility class
    }
};
} // namespace std::experimental

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

inline bool _coro_finished(msvc_frame_prefix* m) noexcept
{
    // expect: coroutine == suspended
    // expect: coroutine != destroyed
    auto* c = reinterpret_cast<clang_frame_prefix*>(m);
    return __builtin_coro_done(c) ? 1 : 0;
}

inline size_t _coro_resume(void* addr)
{
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    __builtin_coro_resume(c);
    return 0;
}

inline void _coro_destroy(void* addr)
{
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    __builtin_coro_destroy(c);
}

#elif _MSC_VER

inline bool _coro_finished(const msvc_frame_prefix* prefix) noexcept
{
    // expect: coroutine == suspended
    // expect: coroutine != destroyed
    return prefix->index == 0;
}

namespace std::experimental
{
//  - Note
//      Helper traits for MSVC's coroutine compilation.
//      The original code is in <experimental/resumable>
template <typename _Ret, typename... _Ts>
struct _Resumable_helper_traits
{
    using promise_type = typename coroutine_traits<_Ret, _Ts...>::promise_type;
    using handle_type = coroutine_handle<promise_type>;

    static promise_type* _Promise_from_frame(void* addr) noexcept
    {
        auto& prom = handle_type::from_address(addr).promise();
        return std::addressof(prom);
    }

    static handle_type _Handle_from_frame(void* _Addr) noexcept
    {
        auto* p = _Promise_from_frame(_Addr);
        return handle_type::from_promise(*p);
    }

    static void _Set_exception(void* addr) noexcept
    {
        auto& prom = handle_type::from_address(addr).promise();
        prom->set_exception(std::current_exception());
    }

    static void _ConstructPromise(void* addr, void* func,
                                  int _HeapElision) noexcept(false)
    {
        auto prefix = static_cast<msvc_frame_prefix*>(addr);
        prefix->factivate = static_cast<procedure_t>(func);

        uint32_t* ptr = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uintptr_t>(prefix) + sizeof(void*));
        *ptr = 2 + (_HeapElision ? 0 : 0x10000);

        auto* prom = _Promise_from_frame(prefix);
        ::new (prom) promise_type();
    }

    static void _DestructPromise(void* addr) noexcept
    {
        auto prefix = static_cast<msvc_frame_prefix*>(addr);
        _Promise_from_frame(prefix)->~promise_type();
    }
};

} // namespace std::experimental

#endif // __clang__ || _MSC_VER

#pragma warning(pop)
#endif // COROUTINE_FRAME_PREFIX_HPP