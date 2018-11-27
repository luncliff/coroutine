// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
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

#ifndef COROUTINE_THREAD_SWITCHING_H
#define COROUTINE_THREAD_SWITCHING_H

#include <coroutine/frame.h>

_INTERFACE_ bool peek_switched(
    std::experimental::coroutine_handle<void>& rh) noexcept(false);

// - Note
//      Routine switching to another thread with MSVC Coroutine
//      and Windows Thread Pool
class switch_to final
{
    uint64_t u64[3]{};

  public:
    _INTERFACE_ explicit switch_to(uint64_t target = 0) noexcept(false);
    _INTERFACE_ ~switch_to() noexcept;

  private:
    switch_to(switch_to&&) noexcept = delete;
    switch_to& operator=(switch_to&&) noexcept = delete;
    switch_to(const switch_to&) noexcept = delete;
    switch_to& operator=(const switch_to&) noexcept = delete;

  public:
    _INTERFACE_ bool ready() const noexcept;
    _INTERFACE_ void suspend(
        std::experimental::coroutine_handle<void> rh) noexcept(false);
    _INTERFACE_ void resume() noexcept;

#pragma warning(disable : 4505)
    bool await_ready() const noexcept
    {
        // redirect
        return this->ready();
    }
    void await_suspend( //
        std::experimental::coroutine_handle<void> rh) noexcept(false)
    {
        // redirect
        return this->suspend(rh);
    }
    void await_resume() noexcept
    {
        // redirect
        return this->resume();
    }
#pragma warning(default : 4505)
};

#endif // COROUTINE_THREAD_SWITCHING_H
