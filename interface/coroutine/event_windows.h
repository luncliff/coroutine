/**
 * @file event_windows.h
 * @brief Awaitable type over system's event object
 * @author github.com/luncliff <luncliff@gmail.com>
 * @copyright CC BY 4.0
 */
#pragma once
#ifndef COROUTINE_AWAITABLE_EVENT_H
#define COROUTINE_AWAITABLE_EVENT_H

#include <coroutine/frame.h>

/**
 * @defgroup Event
 * Helper types to apply `co_await` for event objects.
 * 
 * For Windows, the implementation uses Win32 thread pool.
 * For Linux, it uses Epoll.
 * For Darwin, it uses Kqueue.
 */

namespace coro {
using namespace std;
using namespace std::experimental;

/**
 * @brief Awaitable event type over Win32 thread pool
 * @ingroup Event
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
    void* hobject;

    set_or_cancel(const set_or_cancel&) = delete;
    set_or_cancel(set_or_cancel&&) = delete;
    set_or_cancel& operator=(const set_or_cancel&) = delete;
    set_or_cancel& operator=(set_or_cancel&&) = delete;

  public:
    explicit set_or_cancel(void* target) noexcept(false);
    ~set_or_cancel() noexcept;

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

} // namespace coro

#endif // COROUTINE_AWAITABLE_EVENT_H
