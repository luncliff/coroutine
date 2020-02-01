#include <coroutine/darwin.h>
#include <coroutine/return.h>

namespace coro {

auto read_async(kqueue_owner& kq, uint64_t fd) -> frame_t {
    kevent64_s req{.ident = fd,
                   .filter = EVFILT_READ,
                   .flags = EV_ADD | EV_ENABLE | EV_ONESHOT};
    co_await kq.submit(req);
}

// ...
} // namespace coro
