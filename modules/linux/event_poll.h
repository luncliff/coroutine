// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once
#ifndef LINUX_EVENT_POLL_API_WRAPPER_H
#define LINUX_EVENT_POLL_API_WRAPPER_H

#include <coroutine/yield.hpp>

#include <memory>

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

struct event_poll_t final {
    int fd;
    const size_t capacity;
    std::unique_ptr<epoll_event[]> events;

  public:
    event_poll_t() noexcept(false);
    ~event_poll_t() noexcept;

    void try_add(uint64_t fd, epoll_event& req) noexcept(false);
    void remove(uint64_t fd);
    auto wait(int timeout) noexcept(false) -> coro::enumerable<epoll_event>;
};

#endif // LINUX_EVENT_POLL_API_WRAPPER_H
