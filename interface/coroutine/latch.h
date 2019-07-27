//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once

// clang-format off
#if defined(FORCE_STATIC_LINK)
#   define _INTERFACE_
#   define _HIDDEN_
#elif defined(_MSC_VER) // MSVC or clang-cl
#   define _HIDDEN_
#   ifdef _WINDLL
#       define _INTERFACE_ __declspec(dllexport)
#   else
#       define _INTERFACE_ __declspec(dllimport)
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define _INTERFACE_ __attribute__((visibility("default")))
#   define _HIDDEN_ __attribute__((visibility("hidden")))
#else
#   error "unexpected linking configuration"
#endif
// clang-format on

#ifndef CONCURRENCY_HELPER_LATCH_H
#define CONCURRENCY_HELPER_LATCH_H

#include <atomic>
#include <chrono>

#if __has_include(<Windows.h>) // ... activate VC++ based features ...
namespace coro {

//  An `std::experimental::latch` for fork-join scenario.
//  Its interface might slightly with that of Concurrency TS
class latch final {
    mutable uint64_t ev{};
    atomic_uint64_t ref{};

  public:
    _INTERFACE_ explicit latch(uint32_t count) noexcept(false);
    _INTERFACE_ ~latch() noexcept;

    _INTERFACE_ void count_down_and_wait() noexcept(false);
    _INTERFACE_ void count_down(uint32_t n = 1) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void wait() noexcept(false);
};

} // namespace coro
#elif __has_include(<pthread.h>)

#include <pthread.h>

namespace coro {
using namespace std;
using namespace std::experimental;

//  An `std::experimental::latch` for fork-join scenario.
//  Its interface might slightly with that of Concurrency TS
class latch final {
    atomic_uint64_t ref{};
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};

  private:
    int timed_wait(chrono::microseconds timeout) noexcept;

  public:
    _INTERFACE_ explicit latch(uint32_t count) noexcept(false);
    _INTERFACE_ ~latch() noexcept;

    _INTERFACE_ void count_down_and_wait() noexcept(false);
    _INTERFACE_ void count_down(uint32_t n = 1) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void wait() noexcept(false);
};

} // namespace coro
#endif
#endif // CONCURRENCY_HELPER_LATCH_H