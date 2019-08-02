//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Header to adjust the difference of coroutine frame between compilers
//
//  Reference
//      <experimental/resumable> from Microsoft VC++ (since 2017 Feb.)
//      <experimental/coroutine> from LLVM libcxx (since 6.0)
//      https://github.com/iains/gcc-cxx-coroutines
//
#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__clang__) && defined(_MSC_VER)
//
// case: clang-cl, VC++
//	In this case, override <experimental/resumable>.
//	Since msvc and clang++ uses differnet frame layout,
//  VC++ won't fit clang-cl's code generation.
//  see the implementation below
//
#if defined(_EXPERIMENTAL_RESUMABLE_)
static_assert(false, "This header replaces <experimental/coroutine>"
                     " for clang-cl/VC++. Please remove previous includes.");
#endif
// supporess later includes
#define _EXPERIMENTAL_RESUMABLE_

#elif defined(USE_PORTABLE_COROUTINE_HANDLE) // use this one
//
// case: clang-cl, VC++
// case: msvc, VC++
// case: clang, libc++
//
#else                                        // use default header
//
// case: msvc, VC++
// case: clang, libc++
//	It is safe to use vendor's header.
//	by defining macro variable, user can prevent template redefinition
//
#if __has_include(<coroutine>)               // C++ 20 standard
#include <coroutine>
#elif __has_include(<experimental/coroutine>) // C++ 17 experimetal
#include <experimental/coroutine>
// We don't need to use this portable one.
// Disable the implementation below and use the default
#define COROUTINE_PORTABLE_FRAME_H
#endif
#endif // <coroutine> header

#if defined(__clang__)
static constexpr auto is_clang = true;
static constexpr auto is_msvc = !is_clang;
static constexpr auto is_gcc = !is_clang;

using procedure_t = void(__cdecl*)(void*);

#elif defined(_MSC_VER)
static constexpr auto is_msvc = true;
static constexpr auto is_clang = !is_msvc;
static constexpr auto is_gcc = !is_msvc;

using procedure_t = void(__cdecl*)(void*);

#elif defined(__GNUC__)
static constexpr auto is_gcc = true;
static constexpr auto is_msvc = !is_gcc;
static constexpr auto is_clang = !is_gcc;

// gcc-10 failes when __cdecl is used. declare it without convention
using procedure_t = void (*)(void*);

#else
#error "unexpected compiler. please contact the author"
#endif

template <typename T>
constexpr auto aligned_size_v = ((sizeof(T) + 16u - 1u) & ~(16u - 1u));

// - Note
//      MSVC coroutine frame's prefix
//      Reference <experimental/resumable> for the detail
// - Layout
//      +------------+------------------+--------------------+
//      | Promise(?) | Frame Prefix(16) | Function Locals(?) |
//      +------------+------------------+--------------------+
struct msvc_frame_prefix final {
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
struct clang_frame_prefix final {
    procedure_t factivate;
    procedure_t fdestroy;
};
static_assert(aligned_size_v<clang_frame_prefix> == 16);

// - Note
//      GCC coroutine frame's prefix
// - Layout
//      Unknown
struct gcc_frame_prefix final {
    void* _unknown1;
    void* _unknown2;
};
static_assert(aligned_size_v<gcc_frame_prefix> == 16);

#ifndef COROUTINE_PORTABLE_FRAME_H
#define COROUTINE_PORTABLE_FRAME_H
#pragma warning(push, 4)
#pragma warning(disable : 4455 4494 4577 4619 4643 4702 4984 4988)
#pragma warning(disable : 26490 26481 26476 26429 26409)

#include <type_traits>

// Alternative of `_coro_done` of msvc for this library.
// It is renamed to avoid redefinition
bool _coro_finished(const msvc_frame_prefix*) noexcept;

//
// intrinsic: MSVC
//
extern "C" size_t _coro_resume(void*);
extern "C" void _coro_destroy(void*);
extern "C" size_t _coro_done(void*); // <- leads compiler error

//
// intrinsic: Clang/GCC
//
extern "C" bool __builtin_coro_done(void*);
extern "C" void __builtin_coro_resume(void*);
extern "C" void __builtin_coro_destroy(void*);
// void* __builtin_coro_promise(void* ptr, int align, bool p);

namespace std {
namespace experimental {

// template <typename R, class = void>
// struct coroutine_traits_sfinae {};
//
// template <typename R>
// struct coroutine_traits_sfinae<R, void_t<typename R::promise_type>> {
//    using promise_type = typename R::promise_type;
// };

// traits to enforce `promise_type`, without sfinae consideration.
template <typename ReturnType, typename... Args>
struct coroutine_traits {
    using promise_type = typename ReturnType::promise_type;
};

template <typename PromiseType = void>
class coroutine_handle;

template <>
class coroutine_handle<void> {
  public:
    // This type is exposed
    //  to provide more information for the frame and
    //  to prepare for future adaptation. (especially for gcc family)
    union prefix_t {
        void* v{};
        msvc_frame_prefix* m;
        clang_frame_prefix* c;
        gcc_frame_prefix* g;
    };
    static_assert(sizeof(prefix_t) == sizeof(void*));
    prefix_t prefix;

  public:
    coroutine_handle() noexcept = default;
    ~coroutine_handle() noexcept = default;
    coroutine_handle(coroutine_handle const&) noexcept = default;
    coroutine_handle(coroutine_handle&& rhs) noexcept = default;
    coroutine_handle& operator=(coroutine_handle const&) noexcept = default;
    coroutine_handle& operator=(coroutine_handle&& rhs) noexcept = default;

    explicit coroutine_handle(std::nullptr_t) noexcept : prefix{nullptr} {
    }
    coroutine_handle& operator=(nullptr_t) noexcept {
        prefix.v = nullptr;
        return *this;
    }

    explicit operator bool() const noexcept {
        return prefix.v != nullptr;
    }
    void resume() noexcept(false) {
        if constexpr (is_msvc) {
            _coro_resume(prefix.m);
        } else if constexpr (is_clang) {
            __builtin_coro_resume(prefix.c);
        } else if constexpr (is_gcc) {
            __builtin_coro_resume(prefix.g);
        }
    }
    void destroy() noexcept {
        if constexpr (is_msvc) {
            _coro_destroy(prefix.m);
        } else if constexpr (is_clang) {
            __builtin_coro_destroy(prefix.c);
        } else if constexpr (is_gcc) {
            __builtin_coro_destroy(prefix.g);
        }
    }
    bool done() const noexcept {
        if constexpr (is_msvc) {
            return _coro_finished(prefix.m);
        } else if constexpr (is_clang) {
            return __builtin_coro_done(prefix.c);
        } else if constexpr (is_gcc) {
            return __builtin_coro_done(prefix.g);
        } else {
            return false;
        }
    }

  public:
    constexpr void* address() const noexcept {
        return prefix.v;
    }

    static coroutine_handle from_address(void* addr) noexcept {
        coroutine_handle rh{};
        rh.prefix.v = addr;
        return rh;
    }
};

template <typename PromiseType>
class coroutine_handle : public coroutine_handle<void> {
  public:
    using promise_type = PromiseType;

  private:
    static promise_type* from_frame(prefix_t addr) noexcept {
        if constexpr (is_clang) {
            // calculate the location of the frame's prefix
            auto* prefix = addr.c;
            // for clang, promise is placed just after frame prefix
            // so this line works like `__builtin_coro_promise`,
            auto* promise = reinterpret_cast<promise_type*>(prefix + 1);
            return promise;
        } else if constexpr (is_msvc) {
            auto* ptr = reinterpret_cast<char*>(addr.m);
            // for msvc, promise is placed before frame prefix
            auto* promise = reinterpret_cast<promise_type*>(
                ptr - aligned_size_v<promise_type>);
            return promise;
        } else if constexpr (is_gcc) {
            void* ptr =
                __builtin_coro_promise(addr.g, __alignof(promise_type), false);
            return reinterpret_cast<promise_type*>(ptr);
        }
        // !!! crash !!!
        return nullptr;
    }

  public:
    using coroutine_handle<void>::coroutine_handle;

    coroutine_handle& operator=(nullptr_t) noexcept {
        this->prefix.v = nullptr;
        return *this;
    }
    auto promise() const noexcept -> const promise_type& {
        promise_type* p = from_frame(prefix);
        return *p;
    }
    auto promise() noexcept -> promise_type& {
        promise_type* p = from_frame(prefix);
        return *p;
    }

  public:
    static coroutine_handle from_address(void* addr) noexcept {
        coroutine_handle rh{};
        rh.prefix.v = addr;
        return rh;
    }

    static coroutine_handle from_promise(promise_type& prom) noexcept {
        promise_type* promise = &prom;
        // calculate the location of the coroutine frame prefix
        if constexpr (is_clang) {
            void* prefix =
                reinterpret_cast<char*>(promise) - sizeof(clang_frame_prefix);
            return coroutine_handle::from_address(prefix);
        } else if constexpr (is_msvc) {
            void* prefix =
                reinterpret_cast<char*>(promise) + aligned_size_v<promise_type>;
            return coroutine_handle::from_address(prefix);
        } else if constexpr (is_gcc) {
            void* prefix = __builtin_coro_promise(
                reinterpret_cast<char*>(&prom), __alignof(promise_type), true);
            return coroutine_handle::from_address(prefix);
        }
        return coroutine_handle{};
    }
};
static_assert(sizeof(coroutine_handle<void>) == sizeof(void*));

inline bool operator==(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept {
    return lhs.address() == rhs.address();
}
inline bool operator!=(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept {
    return !(lhs == rhs);
}
inline bool operator<(const coroutine_handle<void> lhs,
                      const coroutine_handle<void> rhs) noexcept {
    return lhs.address() < rhs.address();
}
inline bool operator>(const coroutine_handle<void> lhs,
                      const coroutine_handle<void> rhs) noexcept {
    return !(lhs < rhs);
}
inline bool operator<=(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept {
    return !(lhs > rhs);
}
inline bool operator>=(const coroutine_handle<void> lhs,
                       const coroutine_handle<void> rhs) noexcept {
    return !(lhs < rhs);
}

class suspend_never {
  public:
    constexpr bool await_ready() const noexcept {
        return true;
    }
    constexpr void await_resume() const noexcept {
    }
    void await_suspend(coroutine_handle<void>) const noexcept {
        // Since the class won't suspend, this function won't be invoked
    }
};

class suspend_always {
  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() const noexcept {
    }
    void await_suspend(coroutine_handle<void>) const noexcept {
        // The routine will suspend but the class ignores the given handle
    }
};

} // namespace experimental
} // namespace std

#if defined(__clang__)
//
//  Note
//      VC++ header expects msvc intrinsics. Redirect them to Clang intrinsics.
//      If the project uses libc++ header files, this code won't be a problem
//      because they wont't be used
//  Reference
//      https://clang.llvm.org/docs/LanguageExtensions.html#c-coroutines-support-builtins
//      https://llvm.org/docs/Coroutines.html#example
//

inline bool _coro_finished(msvc_frame_prefix* m) noexcept {
    // expect: coroutine == suspended
    // expect: coroutine != destroyed
    auto* c = reinterpret_cast<clang_frame_prefix*>(m);
    return __builtin_coro_done(c) ? 1 : 0;
}

inline size_t _coro_resume(void* addr) {
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    __builtin_coro_resume(c);
    return 0;
}

inline void _coro_destroy(void* addr) {
    auto* c = reinterpret_cast<clang_frame_prefix*>(addr);
    __builtin_coro_destroy(c);
}

#elif defined(_MSC_VER)

inline bool _coro_finished(const msvc_frame_prefix* prefix) noexcept {
    // expect: coroutine == suspended
    // expect: coroutine != destroyed
    return prefix->index == 0;
}

namespace std::experimental {
// Helper traits for MSVC's coroutine compilation.
// The original code is in <experimental/resumable>
template <typename _Ret, typename... _Ts>
struct _Resumable_helper_traits {
    using promise_type = typename coroutine_traits<_Ret, _Ts...>::promise_type;
    using handle_type = coroutine_handle<promise_type>;

    static promise_type* _Promise_from_frame(void* addr) noexcept {
        auto& prom = handle_type::from_address(addr).promise();
        return &prom;
    }

    static handle_type _Handle_from_frame(void* _Addr) noexcept {
        auto* p = _Promise_from_frame(_Addr);
        return handle_type::from_promise(*p);
    }

    static void _Set_exception(void* addr) noexcept {
        auto& prom = handle_type::from_address(addr).promise();
        prom->set_exception(std::current_exception());
    }

    static void _ConstructPromise(void* addr, void* func,
                                  int _HeapElision) noexcept(false) {
        auto prefix = static_cast<msvc_frame_prefix*>(addr);
        prefix->factivate = static_cast<procedure_t>(func);

        uint32_t* ptr = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uintptr_t>(prefix) + sizeof(void*));
        *ptr = 2 + (_HeapElision ? 0 : 0x10000);

        auto* prom = _Promise_from_frame(prefix);
        ::new (prom) promise_type();
    }

    static void _DestructPromise(void* addr) noexcept {
        auto prefix = static_cast<msvc_frame_prefix*>(addr);
        _Promise_from_frame(prefix)->~promise_type();
    }
};
} // namespace std::experimental

#elif defined(__GNUC__)

inline bool is_suspended(gcc_frame_prefix* g) noexcept {
    return __builtin_coro_is_suspended(g);
}

#endif // __clang__ || _MSC_VER || __GNUC__
#pragma warning(pop)
#endif // COROUTINE_PORTABLE_FRAME_H
