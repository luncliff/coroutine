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
#ifndef LINKABLE_DLL_MACRO
#define LINKABLE_DLL_MACRO

#ifdef _MSC_VER // MSVC
#define _HIDDEN_
#ifdef _WINDLL
#define _INTERFACE_ __declspec(dllexport)
#else
#define _INTERFACE_ __declspec(dllimport)
#endif

#elif __GNUC__ || __clang__ // GCC or Clang
#define _INTERFACE_ __attribute__((visibility("default")))
#define _HIDDEN_ __attribute__((visibility("hidden")))

#else
#error "unexpected compiler"

#endif // compiler check
#endif // LINKABLE_DLL_MACRO

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

  private:
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

  public:
    _INTERFACE_ section(uint16_t spin = 0x600) noexcept(false);
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
    using duration = std::chrono::milliseconds;

  private:
    wait_group(wait_group&) = delete;
    wait_group(wait_group&&) = delete;
    wait_group& operator=(wait_group&) = delete;
    wait_group& operator=(wait_group&&) = delete;

  public:
    _INTERFACE_ wait_group() noexcept(false);
    _INTERFACE_ ~wait_group() noexcept;

  public:
    _INTERFACE_ void add(uint16_t delta) noexcept;
    _INTERFACE_ void done() noexcept;
    _INTERFACE_
    bool wait(duration d = std::chrono::seconds{10}) noexcept(false);
};

enum class thread_id_t : uint64_t;

struct _INTERFACE_ message_t final
{
    union {
        uint32_t u32[2]{};
        uint64_t u64;
        void* ptr;
    };

    bool operator==(const message_t& rhs) const noexcept
    {
        return u64 == rhs.u64;
    }
    bool operator!=(const message_t& rhs) const noexcept
    {
        return u64 != rhs.u64;
    }
};
static_assert(sizeof(message_t) <= sizeof(uint64_t));
static_assert(sizeof(message_t) <= sizeof(void*));

// - Note
//      Get the current thread id
//      This function is identical to `pthread_self` or `GetCurrentThreadId`.
//      But it uses type system to use `post_message`
_INTERFACE_
thread_id_t current_thread_id() noexcept;

// - Note
//      Post a message to the thread with given id
_INTERFACE_
bool post_message(thread_id_t thread_id, message_t msg) noexcept(false);

// - Note
//      Peek a message from current thread's message queue
// - Return
//      message holds `nullptr` if the queue is empty
_INTERFACE_
bool peek_message(message_t& msg) noexcept(false);

_INTERFACE_
bool get_message(message_t& msg,
                 std::chrono::nanoseconds timeout) noexcept(false);

#endif // CONTROL_FLOW_SYNC_H
