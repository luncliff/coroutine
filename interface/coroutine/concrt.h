// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  References
//      https://en.cppreference.com/w/cpp/experimental/concurrency
//		https://en.cppreference.com/w/cpp/experimental/latch
//      https://github.com/alasdairmackintosh/google-concurrency-library
//      https://docs.microsoft.com/en-us/windows/desktop/ProcThread/using-the-thread-pool-functions
//
// ---------------------------------------------------------------------------
#pragma once
// clang-format off
#ifdef USE_STATIC_LINK_MACRO // ignore macro declaration in static build
#   define _INTERFACE_
#   define _HIDDEN_
#else 
#   if defined(_MSC_VER) // MSVC
#       define _HIDDEN_
#       ifdef _WINDLL
#           define _INTERFACE_ __declspec(dllexport)
#       else
#           define _INTERFACE_ __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) || defined(__clang__)
#       define _INTERFACE_ __attribute__((visibility("default")))
#       define _HIDDEN_ __attribute__((visibility("hidden")))
#   else
#       error "unexpected compiler"
#   endif // compiler check
#endif
// clang-format on

#ifndef EXPERIMENTAL_CONCURRENCY_TS_ADAPTER_H
#define EXPERIMENTAL_CONCURRENCY_TS_ADAPTER_H

#include <cstddef>
#include <system_error>

#include <coroutine/return.h>

namespace concrt
{

//	An opaque implementation of `std::experimental::latch`.
//	Useful for fork-join scenario. Its interface might slightly different
//  with latch in the TS
class latch
{
    std::byte storage[128]{};

  public:
    latch(latch&) = delete;
    latch(latch&&) = delete;
    latch& operator=(latch&) = delete;
    latch& operator=(latch&&) = delete;

    _INTERFACE_ explicit latch(uint32_t count) noexcept(false);
    _INTERFACE_ ~latch() noexcept;

    _INTERFACE_ void count_down_and_wait() noexcept(false);
    _INTERFACE_ void count_down(uint32_t n = 1) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void wait() noexcept(false);
};

} // namespace concrt

#if defined(_MSC_VER) // ... VC++ only features ...
// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <sdkddkver.h>
#include <threadpoolapiset.h>
// clang-format on

namespace concrt
{
using namespace std;
using namespace std::experimental;

//	Move into the win32 thread pool and continue the routine
class ptp_work final : public suspend_always
{
    static void __stdcall resume_on_thread_pool( //
        PTP_CALLBACK_INSTANCE, PVOID, PTP_WORK);

  public:
    // The `ctx` must be address of coroutine frame
    _INTERFACE_ auto suspend(coroutine_handle<void> coro) noexcept -> uint32_t;

    // Lazy code generation in importing code by header usage.
    void await_suspend(coroutine_handle<void> coro) noexcept(false)
    {
        if (const auto ec = suspend(coro))
            throw system_error{static_cast<int>(ec), system_category(),
                               "CreateThreadpoolWork"};
    }
};

// standard lockable concept with win32 criticial section
class section final : CRITICAL_SECTION
{
  public:
    _INTERFACE_ section() noexcept(false);
    _INTERFACE_ ~section() noexcept;
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};

} // namespace concrt

#else // ... pthread based features ...

#include <pthread.h>

namespace concrt
{
using namespace std;
using namespace std::experimental;

class pthread_section final
{
    pthread_rwlock_t rwlock{};

  public:
    _INTERFACE_ pthread_section() noexcept(false);
    _INTERFACE_ ~pthread_section() noexcept;
    pthread_section(pthread_section&) = delete;
    pthread_section(pthread_section&&) = delete;
    pthread_section& operator=(pthread_section&) = delete;
    pthread_section& operator=(pthread_section&&) = delete;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};

} // namespace concrt
#endif

#endif // EXPERIMENTAL_CONCURRENCY_TS_ADAPTER_H
