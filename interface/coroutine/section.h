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

#ifndef CONCURRENCY_HELPER_SECTION_H
#define CONCURRENCY_HELPER_SECTION_H

#if __has_include(<Windows.h>) // ... activate VC++ based features ...

// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <synchapi.h>
// clang-format on

namespace coro {
using namespace std;
using namespace std::experimental;

//  Standard lockable with win32 criticial section
class section final : CRITICAL_SECTION, no_copy_move {
  public:
    _INTERFACE_ section() noexcept(false);
    _INTERFACE_ ~section() noexcept;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};
static_assert(sizeof(section) == sizeof(CRITICAL_SECTION));

} // namespace coro

#elif __has_include(<pthread.h>)

#include <pthread.h>

namespace coro {
using namespace std;
using namespace std::experimental;

//  Standard lockable with pthread reader writer lock
class section final : no_copy_move {
    pthread_rwlock_t rwlock;

  public:
    _INTERFACE_ section() noexcept(false);
    _INTERFACE_ ~section() noexcept;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};

} // namespace coro

#endif
#endif // CONCURRENCY_HELPER_SECTION_H