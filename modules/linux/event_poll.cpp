// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./event_poll.h"

#include <system_error>
using namespace std;

event_poll_t::event_poll_t() noexcept(false)
    : epfd{-1},
      // use 2 page for polling
      capacity{2 * getpagesize() / sizeof(epoll_event)},
      events{make_unique<epoll_event[]>(capacity)} {
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0)
        throw system_error{errno, system_category(), "epoll_create1"};
}
event_poll_t::~event_poll_t() noexcept {
    close(epfd);
}

void event_poll_t::try_add(uint64_t _fd, epoll_event& req) noexcept(false) {
    int op = EPOLL_CTL_ADD, ec = 0;
TRY_OP:
    ec = epoll_ctl(epfd, op, _fd, &req);
    if (ec == 0)
        return;
    if (errno == EEXIST) {
        op = EPOLL_CTL_MOD; // already exists. try with modification
        goto TRY_OP;
    }

    throw system_error{errno, system_category(), "epoll_ctl"};
}

void event_poll_t::remove(uint64_t _fd) {
    epoll_event req{}; // just prevent non-null input
    const auto ec = epoll_ctl(epfd, EPOLL_CTL_DEL, _fd, &req);
    if (ec != 0)
        throw system_error{errno, system_category(), "epoll_ctl"};
}

auto event_poll_t::wait(int timeout) noexcept(false)
    -> coro::enumerable<epoll_event> {
    auto count = epoll_wait(epfd, events.get(), capacity, timeout);
    if (count == -1)
        throw system_error{errno, system_category(), "epoll_wait"};

    for (auto i = 0; i < count; ++i) {
        co_yield events[i];
    }
}
