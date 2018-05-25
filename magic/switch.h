// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Note
//      Thread switching for the coroutines
//
//  Reference
//      - Windows via C/C++ (5th Edition)
//      - MSDN | Thread Pool API
//      - CppCon 2016 | Putting Coroutines to Work with the Windows Runtime
//          by Kenny Kerr & James McNellis
//
// ---------------------------------------------------------------------------
#ifndef _MAGIC_COROUTINE_SWITCH_H_
#define _MAGIC_COROUTINE_SWITCH_H_

#include <magic/linkable.h>

#include <experimental/coroutine>
#include <experimental/generator>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <threadpoolapiset.h>

namespace magic {
namespace stdex = std::experimental;

// - Note
//      Take resumeable handle from a specific thread's MSG queue
_INTERFACE_ bool peek(stdex::coroutine_handle<> &rh) noexcept;

// - Note
//      Take resumeable handle from a specific thread's MSG queue
_INTERFACE_ bool get(stdex::coroutine_handle<> &rh) noexcept;

// - Note
//      Routine switching to another thread with MSVC Coroutine
//      and Windows Thread Pool
class _INTERFACE_ switch_to {
  // non-zero : Specific thread's queue
  //     zero : Windows Thread Pool
  DWORD thread;
  // Windows Thread Pool's Work Item
  PTP_WORK work;

public:
  explicit switch_to(uint32_t target = 0) noexcept;
  switch_to(switch_to &&) noexcept;
  switch_to &operator=(switch_to &&) noexcept;
  ~switch_to() noexcept;

private:
  switch_to(const switch_to &) noexcept = delete;
  switch_to &operator=(const switch_to &) noexcept = delete;

public:
  bool ready() const noexcept;
  // - Throws
  //      std::runtime_error
  void suspend(stdex::coroutine_handle<> rh) noexcept(false);
  void resume() noexcept;

private:
  // - Note
  //      Thread Pool Callback. Expect `noexcept` operation
  static void CALLBACK onWork(PTP_CALLBACK_INSTANCE pInstance, PVOID pContext,
                              PTP_WORK pWork) noexcept;
};

#pragma warning(disable : 4505)

static bool await_ready(const switch_to &awaitable) noexcept {
  return awaitable.ready();
}
static decltype(auto)
await_suspend(switch_to &awaitable,
              stdex::coroutine_handle<> rh) noexcept(false) {
  return awaitable.suspend(rh);
}
static decltype(auto) await_resume(switch_to &awaitable) noexcept {
  return awaitable.resume();
}

#pragma warning(default : 4505)

} // namespace magic
#endif
