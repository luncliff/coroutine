// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Golang-style synchronization with system event API
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

#ifndef COROUTINE_SYNC_UTIL_H
#define COROUTINE_SYNC_UTIL_H

#include <chrono> // for timeout

// - Note
//      sync type for fork-join scenario
// - See Also
//      package `sync` in Go Language
//      https://golang.org/pkg/sync/#WaitGroup
class _INTERFACE_ wait_group final
{
  public:
    using duration = std::chrono::milliseconds;

  private:
    // reserve enough size to provide platform compatibility
    const uint64_t storage[16]{};

  public:
    wait_group(wait_group&) = delete;
    wait_group(wait_group&&) = delete;
    wait_group& operator=(wait_group&) = delete;
    wait_group& operator=(wait_group&&) = delete;

    wait_group() noexcept(false);
    ~wait_group() noexcept;

    void add(uint16_t delta) noexcept;
    void done() noexcept;
    bool wait(duration d) noexcept(false);
};

#endif // COROUTINE_SYNC_UTIL_H
