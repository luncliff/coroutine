// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Note
//      Utility for synchronization
//      Basic Lockable with Win32 Critical Section
//
//  Reference
//      - MSDN
//      - Windows via C/C++ (5th Edition)
//
// ---------------------------------------------------------------------------
#ifndef _MAGIC_FLOW_SYNC_H_
#define _MAGIC_FLOW_SYNC_H_

#include <magic/linkable.h>

#include <atomic>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // System API

namespace magic {

// - Note
//      Basic Lockable with Win32 Critical Section
class section : public CRITICAL_SECTION {
  section(section &) = delete;
  section(section &&) = delete;
  section &operator=(section &) = delete;
  section &operator=(section &&) = delete;

public:
  _INTERFACE_ explicit section(uint16_t spin = 0x800) noexcept;
  _INTERFACE_ ~section() noexcept;

  _INTERFACE_ bool try_lock() noexcept;
  _INTERFACE_ void lock() noexcept;
  _INTERFACE_ void unlock() noexcept;
};

// - Note
//      WaitGroup with Win32 Event
//      The type is designed to support alertable thread
// - See Also
//      package `sync` in Go Language
//      https://golang.org/pkg/sync/#WaitGroup
class wait_group final {
  HANDLE eve = INVALID_HANDLE_VALUE;
  std::atomic<uint32_t> ref = 0;

private:
  wait_group(wait_group &) = delete;
  wait_group(wait_group &&) = delete;
  wait_group &operator=(wait_group &) = delete;
  wait_group &operator=(wait_group &&) = delete;

public:
  // - Throws
  //      std::runtime_error
  _INTERFACE_ wait_group() noexcept(false);
  _INTERFACE_ ~wait_group() noexcept;

  _INTERFACE_ void add(uint32_t count) noexcept;
  _INTERFACE_ void done() noexcept;
  _INTERFACE_ void wait(uint32_t timeout = INFINITE) noexcept;
};

} // namespace magic

#endif // _MAGIC_FLOW_SYNC_H_
