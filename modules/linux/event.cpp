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

event::event() noexcept(false) : id{}, coro{} {
    const auto fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1)
        throw system_error{errno, system_category(), "eventfd"};

    id = static_cast<decltype(id)>(fd);
}
event::~event() noexcept {
    if (id)
        close(id);
}
bool event::is_ready() noexcept {
    return id == 0; // already siganled
}

void event::on_resume() noexcept {
    // nothing to do here ...
}

// Reference
//  https://github.com/grpc/grpc/blob/master/src/core/lib/iomgr/is_epollexclusive_available.cc
void event::on_suspend(task t) noexcept(false) {
    this->coro = t;
    // printf("%p \n", coro.address());
    // report when read is available
    epoll_event req{};
    req.events = EPOLLET | EPOLLIN | EPOLLEXCLUSIVE | EPOLLONESHOT;
    req.data.fd = id;

    // throws if epoll_ctl fails
    events.try_add(id, req);
}

void event::set() noexcept(false) {
    // it is not suspended
    if (static_cast<bool>(coro) == false) {
        close(id);
        id = 0;
        return;
    }
    // suspended. signal the eventfd
    auto sz = write(id, &coro, sizeof(coro));
    if (sz == -1)
        throw system_error{errno, system_category(), "write"};
}

auto signaled_event_tasks() noexcept(false) -> coro::enumerable<event::task> {
    event::task t{};

    // fetch immediately
    for (auto e : events.wait(0)) {
        auto sz = read(e.data.fd, &t, sizeof(t));
        if (sz == -1)
            throw system_error{errno, system_category(), "read"};

        co_yield t;
    }
    co_return;
}

} // namespace concrt