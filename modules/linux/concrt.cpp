// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>

#include <sys/eventfd.h>

#include "./event_poll.h"

namespace concrt {

event_poll_t events{};

event::event() noexcept(false) : id{} {
    const auto fd = eventfd(0, EFD_CLOEXEC);
    if (fd == -1)
        throw system_error{errno, system_category(), "eventfd"};

    id = static_cast<decltype(id)>(fd);
}
event::~event() noexcept {
    close(id);
}

void event::on_suspend(task t) noexcept(false) {
    internal = t.address();
    // report when read is available
    epoll_event req{};
    req.events = EPOLLIN;
    req.data.fd = id;

    // throws if epoll_ctl fails
    events.try_add(id, req);
}

void event::set() noexcept(false) {
    const auto sz = write(id, &internal, sizeof(internal));
    if (sz == -1)
        throw system_error{errno, system_category(), "write"};
}

auto signaled_event_tasks() noexcept(false) -> coro::enumerable<event::task> {
    event::task task{};

    // fetch immediately
    for (auto e : events.wait(0)) {
        const auto sz = read(e.data.fd, &task, sizeof(task));
        if (sz == -1)
            throw system_error{errno, system_category(), "read"};

        puts("signaled_event_tasks");

        co_yield task;
    }
    co_return;
}

} // namespace concrt