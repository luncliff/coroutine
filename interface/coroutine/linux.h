/**
 * @file coroutine/linux.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 */
#ifndef COROUTINE_SYSTEM_WRAPPER_H
#define COROUTINE_SYSTEM_WRAPPER_H
#if not(defined(__linux__))
#error "expect Linux platform for this file"
#endif

#include <coroutine/frame.h>
#include <gsl/gsl>
#include <sys/epoll.h> // for Linux epoll

/**
 * @defgroup Linux
 */

namespace coro {

/**
 * @brief RAII wrapping for epoll file descriptor
 * @ingroup Linux
 */
class epoll_owner final {
    int64_t epfd;

  public:
    /**
     * @brief create a fd with `epoll`. Throw if the function fails.
     * @see kqeueue
     * @throw system_error
     */
    epoll_owner() noexcept(false);
    /**
     * @brief close the current epoll file descriptor
     */
    ~epoll_owner() noexcept;
    epoll_owner(const epoll_owner&) = delete;
    epoll_owner(epoll_owner&&) = delete;
    epoll_owner& operator=(const epoll_owner&) = delete;
    epoll_owner& operator=(epoll_owner&&) = delete;

  public:
    /**
     * @brief bind the fd to epoll
     * @param fd
     * @param req 
     * @see epoll_ctl
     * @throw system_error
     */
    void try_add(uint64_t fd, epoll_event& req) noexcept(false);

    /**
     * @brief unbind the fd to epoll
     * @param fd 
     * @see epoll_ctl
     */
    void remove(uint64_t fd);

    /**
     * @brief fetch all events for the given kqeueue descriptor
     * @param wait_ms millisecond to wait
     * @param list 
     * @return ptrdiff_t 
     * @see epoll_wait
     * @throw system_error
     * 
     * Timeout is not an error for this function
     */
    ptrdiff_t wait(uint32_t wait_ms,
                   gsl::span<epoll_event> list) noexcept(false);

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
     * ```cpp
     * auto edge_in_async(epoll_owner& ep, int64_t fd) -> frame_t {
     *     epoll_event req{};
     *     req.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
     *     req.data.ptr = nullptr;
     *     co_await ep.submit(fd, req);
     * }
     * ```
     */
    [[nodiscard]] auto submit(int64_t fd, epoll_event& req) noexcept {
        class awaiter final : public suspend_always {
            epoll_owner& ep;
            int64_t fd;
            epoll_event& req;

          public:
            constexpr awaiter(epoll_owner& _ep, int64_t _fd, epoll_event& _req)
                : ep{_ep}, fd{_fd}, req{_req} {
            }

          public:
            void await_suspend(coroutine_handle<void> coro) noexcept(false) {
                if (req.data.ptr == nullptr)
                    req.data.ptr = coro.address();
                return ep.try_add(fd, req);
            }
        };
        return awaiter{*this, fd, req};
    }
};

/**
 * @brief RAII + stateful `eventfd`
 * @see https://github.com/grpc/grpc/blob/master/src/core/lib/iomgr/is_epollexclusive_available.cc
 * @ingroup Linux
 * 
 * If the object is signaled(`set`), 
 * the bound `epoll_owner` will yield suspended coroutine through `epoll_event`'s user data.
 * 
 * Its object can be `co_await`ed multiple times
 */
class event final {
    uint64_t state;

  public:
    event() noexcept(false);
    ~event() noexcept;
    event(const event&) = delete;
    event(event&&) = delete;
    event& operator=(const event&) = delete;
    event& operator=(event&&) = delete;

    uint64_t fd() const noexcept;
    bool is_set() const noexcept;
    void set() noexcept(false);
    void reset() noexcept(false);
};

/**
 * @brief Bind the given `event`(`eventfd`) to `epoll_owner`(Epoll)
 * 
 * @param ep  epoll_owner
 * @param efd event
 * @see event
 * @return awaitable struct for the binding
 * @ingroup Linux
 */
auto wait_in(epoll_owner& ep, event& efd) {
    class awaiter : epoll_event {
        epoll_owner& ep;
        event& efd;

      public:
        /**
         * @brief Prepares one-time registration
         */
        awaiter(epoll_owner& _ep, event& _efd) noexcept
            : epoll_event{}, ep{_ep}, efd{_efd} {
            this->events = EPOLLET | EPOLLIN | EPOLLONESHOT;
        }

        bool await_ready() const noexcept {
            return efd.is_set();
        }
        /**
         * @brief Wait for `write` to given `eventfd`
         */
        void await_suspend(coroutine_handle<void> coro) noexcept(false) {
            this->data.ptr = coro.address();
            return ep.try_add(efd.fd(), *this);
        }
        /**
         * @brief Reset the given event object when resumed
         */
        void await_resume() noexcept {
            return efd.reset();
        }
    };
    return awaiter{ep, efd};
}

} // namespace coro

#endif // COROUTINE_SYSTEM_WRAPPER_H
