/**
 * @brief Add more forward declarations for older versions of STL with C++ 17
 * @todo ClangCL support
 */
#pragma once
#if defined(_MSC_VER) // Microsoft STL
#if __cplusplus >= 202000L
#include <coroutine>

#elif __cplusplus >= 201700L
#include <experimental/coroutine> // C++ 17 (Coroutines TS)

#if !__builtin_coro_noop
// 17.12.4, no-op coroutines
namespace std::experimental {

// STRUCT noop_coroutine_handle
struct noop_coroutine_promise {};
using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

// STRUCT coroutine_handle<noop_coroutine_promise>
template <>
struct coroutine_handle<noop_coroutine_promise> : public coroutine_handle<void> {
    friend noop_coroutine_handle noop_coroutine() noexcept;

  private:
    explicit coroutine_handle(coroutine_handle<void> handle) noexcept;
};

// 17.12.4.3
[[nodiscard]] noop_coroutine_handle noop_coroutine() noexcept;

}; // namespace std::experimental
#endif // !__builtin_coro_noop

#else
#error "Unexpected __cplusplus value. Expect higher than 201700L"

#endif

#elif defined(__clang__) // LLVM libc++
#if __has_include(<coroutine>) && (__cplusplus >= 202000L)
#include <coroutine>

#elif __cplusplus >= 201700L
#include <experimental/coroutine>
#endif

#endif // defined(_MSC_VER) || defined(__clang__)
