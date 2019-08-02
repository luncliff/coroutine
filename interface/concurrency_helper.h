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

#ifndef CONCURRENCY_HELPER_H
#define CONCURRENCY_HELPER_H

#include <atomic>
#include <chrono>
#include <future>

#if __has_include(<Windows.h>)
#include <Windows.h>
#include <synchapi.h>

//  Standard lockable with win32 criticial section
class section final : CRITICAL_SECTION {
    section(const section&) = delete;
    section(section&&) = delete;
    section& operator=(const section&) = delete;
    section& operator=(section&&) = delete;

  public:
    _INTERFACE_ section() noexcept(false);
    _INTERFACE_ ~section() noexcept;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};
static_assert(sizeof(section) == sizeof(CRITICAL_SECTION));

//  An `std::experimental::latch` for fork-join scenario.
//  Its interface might slightly with that of Concurrency TS
class latch final {
    mutable HANDLE ev{};
    std::atomic_uint64_t ref{};

  public:
    _INTERFACE_ explicit latch(uint32_t count) noexcept(false);
    _INTERFACE_ ~latch() noexcept;

    _INTERFACE_ void count_down_and_wait() noexcept(false);
    _INTERFACE_ void count_down(uint32_t n = 1) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void wait() noexcept(false);
};

#elif __has_include(<pthread.h>)
#include <pthread.h>

//  Standard lockable with pthread reader writer lock
class section final {
    pthread_rwlock_t rwlock;

  public:
    _INTERFACE_ section() noexcept(false);
    _INTERFACE_ ~section() noexcept;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};

//  An `std::experimental::latch` for fork-join scenario.
//  Its interface might slightly with that of Concurrency TS
class latch final {
    std::atomic_uint64_t ref{};
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};

  private:
    int timed_wait(std::chrono::nanoseconds timeout) noexcept;

  public:
    _INTERFACE_ explicit latch(uint32_t count) noexcept(false);
    _INTERFACE_ ~latch() noexcept;

    _INTERFACE_ void count_down_and_wait() noexcept(false);
    _INTERFACE_ void count_down(uint32_t n = 1) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void wait() noexcept(false);
};

#endif
#endif // CONCURRENCY_HELPER_H
