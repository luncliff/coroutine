// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Utility for synchronization over System API
//      Lockable, Signal, Message Queue...
//
//  Reference
//      - MSDN
//      - Windows via C/C++ (5th Edition)
//
// ---------------------------------------------------------------------------
#pragma once

#ifdef USE_STATIC_LINK_MACRO // clang-format off
#   define _INTERFACE_
#   define _HIDDEN_
#else // ignore macro declaration in static build
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
#endif // clang-format on

#ifndef CONTROL_FLOW_SYNC_H
#define CONTROL_FLOW_SYNC_H

#include <chrono> // for timeout
#include <mutex>  // for lockable concept

// - Note
//      Basic lockable for criticial section
class section final
{
    // reserve enough size to provide platform compatibility
    const uint64_t storage[16]{};

  public:
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

    _INTERFACE_ section() noexcept(false);
    _INTERFACE_ ~section() noexcept;

    _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept(false);
    _INTERFACE_ void unlock() noexcept(false);
};

// - Note
//      Golang-style synchronization with system event
// - See Also
//      package `sync` in Go Language
//      https://golang.org/pkg/sync/#WaitGroup
class wait_group final
{
    // reserve enough size to provide platform compatibility
    const uint64_t storage[16]{};

  public:
    using duration = std::chrono::microseconds;

  public:
    wait_group(wait_group&) = delete;
    wait_group(wait_group&&) = delete;
    wait_group& operator=(wait_group&) = delete;
    wait_group& operator=(wait_group&&) = delete;

    _INTERFACE_ wait_group() noexcept(false);
    _INTERFACE_ ~wait_group() noexcept;

    _INTERFACE_ void add(uint16_t delta) noexcept;
    _INTERFACE_ void done() noexcept;
    _INTERFACE_ bool wait(duration d) noexcept(false);
};

struct _INTERFACE_ message_t final
{
    union {
        uint32_t u32[2]{};
        uint64_t u64;
        void* ptr;
    };
};
static_assert(sizeof(message_t) == sizeof(uint64_t));

class _INTERFACE_ messaging_queue_t
{
  public:
    using duration = std::chrono::microseconds;

  public:
    virtual ~messaging_queue_t() noexcept = default;

    virtual bool post(message_t msg) noexcept = 0;
    virtual bool peek(message_t& msg) noexcept = 0;
    virtual bool wait(message_t& msg, duration timeout) noexcept = 0;
};

_INTERFACE_ auto create_message_queue() noexcept(false)
    -> std::unique_ptr<messaging_queue_t>;

enum class thread_id_t : uint64_t;

// - Note
//      Get the current thread id
//      This function is identical to `pthread_self` or `GetCurrentThreadId`.
//      But it uses type system to use `post_message`
_INTERFACE_
thread_id_t current_thread_id() noexcept;

#endif // CONTROL_FLOW_SYNC_H
