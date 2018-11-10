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

#include <atomic>
#include <cstdint>
#include <mutex>

// - Note
//      Basic Lockable with Win32 Critical Section
class section final
{
    std::uint64_t u64[8]{};

  private:
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

  public:
    _INTERFACE_ section(std::uint16_t spin = 0x600) noexcept;
    _INTERFACE_ ~section() noexcept;

    [[nodiscard]] _INTERFACE_ bool try_lock() noexcept;
    _INTERFACE_ void lock() noexcept;
    _INTERFACE_ void unlock() noexcept;
};

// - Note
//      WaitGroup with Event Handling
//      The type is designed to support alertable thread
// - See Also
//      package `sync` in Go Language
//      https://golang.org/pkg/sync/#WaitGroup
class wait_group final
{
    void* event;
    std::atomic<uint32_t> count{};

  private:
    wait_group(wait_group&) = delete;
    wait_group(wait_group&&) = delete;
    wait_group& operator=(wait_group&) = delete;
    wait_group& operator=(wait_group&&) = delete;

  public:
    // - Throws
    //      std::system_error
    _INTERFACE_ wait_group() noexcept(false);
    _INTERFACE_ ~wait_group() noexcept;

    _INTERFACE_ void add(std::uint32_t count) noexcept;
    _INTERFACE_ void done() noexcept;
    // - Note
    //      Wait for given milisec
    // - Throws
    //      std::system_error
    _INTERFACE_ void wait(std::uint32_t timeout = 10'000) noexcept(false);
};

union message_t final {
    std::uint64_t u64{};
    void* ptr;
    std::uint32_t u32[2];
};
// static_assert(std::atomic<message_t>::is_always_lock_free);
static_assert(sizeof(message_t) <= sizeof(uint64_t));

// - Note
//      Post a message to the thread with given id
_INTERFACE_ void post_message(std::uint32_t thread_id,
                              message_t msg) noexcept(false);

// - Note
//      Peek a message from the designated thread's message queue
// - Return
//      message holds `nullptr` if the queue is empty
_INTERFACE_ bool peek_message(std::uint32_t thread_id,
                              message_t& msg) noexcept(false);

#endif // CONTROL_FLOW_SYNC_H
