//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
#ifndef DARWIN_KERNEL_QUEUE_API_WRAPPER_H
#define DARWIN_KERNEL_QUEUE_API_WRAPPER_H

#include <coroutine/yield.hpp>

#include <memory>

#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

namespace coro {

struct kernel_queue_t final {
    int kqfd;
    const size_t capacity;
    std::unique_ptr<kevent64_s[]> events;

  public:
    kernel_queue_t() noexcept(false);
    ~kernel_queue_t() noexcept;

    void change(kevent64_s& req) noexcept(false);
    auto wait(const timespec& ts) noexcept(false)
        -> coro::enumerable<kevent64_s>;
};

} // namespace coro

#endif // DARWIN_KERNEL_QUEUE_API_WRAPPER_H