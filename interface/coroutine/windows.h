/**
 * @file coroutine/windows.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 */
#pragma once
#ifndef COROUTINE_SYSTEM_WRAPPER_H
#define COROUTINE_SYSTEM_WRAPPER_H
#if __has_include(<Windows.h>)
#include <Windows.h>
#else
#error "expect Windows platform for this file"
#endif

#include <coroutine/frame.h>
#include <system_error>

/**
 * @defgroup Windows
 * Most of the implementation use Win32 thread pool.
 */

namespace coro {

/**
 * @brief Awaitable event type over Win32 thread pool
 * @ingroup Windows
 * 
 * Its object can be `co_await`ed only once.
 * The purpose of such design is to encourage use of short functions rather
 * than containing multiple objects in the function's body
 * 
 * It uses INFINITE wait. 
 * So its user must sure one of `SetEvent(hEvent)` or `sc.unregister()` 
 * happens after `co_await`.
 * 
 */
class set_or_cancel final {
    /** @brief object for wait register/unregister */
    HANDLE hobject;

  public:
    explicit set_or_cancel(HANDLE target) noexcept(false);
    ~set_or_cancel() noexcept = default;
    set_or_cancel(const set_or_cancel&) = delete;
    set_or_cancel(set_or_cancel&&) = delete;
    set_or_cancel& operator=(const set_or_cancel&) = delete;
    set_or_cancel& operator=(set_or_cancel&&) = delete;

  private:
    /**
     * @brief Resume the coroutine in the thread pool to wait for the event object
     * @see https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-registerwaitforsingleobject
     * @todo can we use `WT_EXECUTEINWAITTHREAD` for this type?
     */
    void suspend(coroutine_handle<void>) noexcept(false);

  public:
    /**
     * @brief cancel the event waiting
     * @return uint32_t 
     * @see https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-unregisterwait
     */
    uint32_t unregister() noexcept;

    constexpr bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        return suspend(coro);
    }
    uint32_t await_resume() noexcept {
        return unregister();
    }
};

/**
 * @brief Move into the win32 thread pool and continue the routine
 * @ingroup Windows
 * @see CreateThreadpoolWork
 * @see SubmitThreadpoolWork
 * @see CloseThreadpoolWork
 */
class continue_on_thread_pool final {
    /**
     * @see CloseThreadpoolWork
     */
    static void __stdcall resume_on_thread_pool(PTP_CALLBACK_INSTANCE, PVOID,
                                                PTP_WORK);
    /**
     * @see CreateThreadpoolWork
     * @see SubmitThreadpoolWork
     */
    uint32_t create_and_submit_work(coroutine_handle<void>) noexcept;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
        // nothing to do for this implementation
    }

    /**
     * @brief Try to submit the coroutine to thread pool
     * @param coro
     * @throw system_error
     */
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        if (const auto ec = create_and_submit_work(coro))
            throw system_error{static_cast<int>(ec), system_category(),
                               "CreateThreadpoolWork"};
    }
};

/**
 * @brief Move into the designated thread's APC queue and continue the routine
 * @ingroup Windows
 * @see QueueUserAPC
 * @see OpenThread
 */
class continue_on_apc final {
    static void __stdcall resume_on_apc(ULONG_PTR);

    /**
     * @see QueueUserAPC
     * @return uint32_t error code from `QueueUserAPC` function
     */
    uint32_t queue_user_apc(coroutine_handle<void>) noexcept;

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() noexcept {
    }

    /**
     * @brief Try to submit the coroutine to the thread's APC queue
     * @param coro
     * @throw system_error
     */
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        if (const auto ec = queue_user_apc(coro))
            throw system_error{static_cast<int>(ec), system_category(),
                               "QueueUserAPC"};
    }

  public:
    /**
     * @param hThread Target thread's handle
     * @see OpenThread
     */
    explicit continue_on_apc(HANDLE hThread) noexcept : thread{hThread} {
    }

  private:
    HANDLE thread;
};

} // namespace coro

#endif // COROUTINE_SYSTEM_WRAPPER_H
