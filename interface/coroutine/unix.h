/**
 * @file coroutine/unix.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 */
#ifndef COROUTINE_SYSTEM_WRAPPER_H
#define COROUTINE_SYSTEM_WRAPPER_H
#if not(defined(unix) or defined(__APPLE__) or defined(__FreeBSD__))
#error "expect UNIX platform for this file"
#endif

#include <coroutine/frame.h>
#include <gsl/gsl>
#include <sys/event.h> // for BSD kqueue

/**
 * @defgroup BSD
 */

namespace coro {
using namespace std;
using namespace std::experimental;

/**
 * @brief RAII wrapping for kqueue file descriptor
 * @ingroup BSD
 */
class kqueue_owner final {
    int64_t kqfd;

  public:
    /**
     * @brief create a fd with `kqueue`. Throw if the function fails.
     * @see kqeueue
     * @throw system_error
     */
    kqueue_owner() noexcept(false);
    /**
     * @brief close the current kqueue file descriptor
     */
    ~kqueue_owner() noexcept;
    kqueue_owner(const kqueue_owner&) = delete;
    kqueue_owner(kqueue_owner&&) = delete;
    kqueue_owner& operator=(const kqueue_owner&) = delete;
    kqueue_owner& operator=(kqueue_owner&&) = delete;

  public:
    /**
     * @brief bind the event to kqueue
     * @param req 
     * @see kevent64
     * @throw system_error
     * 
     * The function is named `change` because 
     * the given argument is used for 'change list' fo `kqueue64`
     */
    void change(kevent64_s& req) noexcept(false);

    /**
     * @brief fetch all events for the given kqeueue descriptor
     * @param wait_time 
     * @param list 
     * @return ptrdiff_t 
     * @see kevent64
     * @throw system_error
     * 
     * The function is named `events` because 
     * the given argument is used for 'event list' fo `kqueue64`
     * 
     * Timeout is not an error for this function
     */
    ptrdiff_t events(const timespec& wait_time,
                     gsl::span<kevent64_s> list) noexcept(false);

  public:
    /**
     * @brief return temporary awaitable object for given event
     * @param req input for `change` operation 
     * @see change
     * 
     * There is no guarantee of reusage of returned awaiter object
     * When it is awaited, and `req.udata` is null(0),
     * the value is set to `coroutine_handle<void>`
     * 
     * @code
     * auto read_async(kqueue_owner& kq, uint64_t fd) -> frame_t {
     *     kevent64_s req{.ident = fd,
     *                    .filter = EVFILT_READ,
     *                    .flags = EV_ADD | EV_ENABLE | EV_ONESHOT};
     *     co_await kq.submit(req);
     *     // ...
     *     co_await kq.submit(req);
     *     // ...
     * }
     * @endcode
     */
    [[nodiscard]] auto submit(kevent64_s& req) noexcept {
        class awaiter final : public suspend_always {
            kqueue_owner& kq;
            kevent64_s& req;

          public:
            constexpr awaiter(kqueue_owner& _kq, kevent64_s& _req)
                : kq{_kq}, req{_req} {
            }

          public:
            void await_suspend(coroutine_handle<void> coro) noexcept(false) {
                if (req.udata == 0)
                    req.udata = reinterpret_cast<uint64_t>(coro.address());
                return kq.change(req);
            }
        };
        return awaiter{*this, req};
    }
};

} // namespace coro

#endif // COROUTINE_SYSTEM_WRAPPER_H
