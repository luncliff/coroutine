/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/unix.h>
#include <system_error>
#include <unistd.h>

namespace coro {

kqueue_owner::kqueue_owner() noexcept(false) : kqfd{kqueue()} {
    if (kqfd < 0)
        throw std::system_error{errno, std::system_category(), "kqueue"};
}
kqueue_owner::~kqueue_owner() noexcept {
    close(kqfd);
}

void kqueue_owner::change(kevent64_s& req) noexcept(false) {
    // attach the event config
    auto ec = kevent64(kqfd, &req, 1, nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw std::system_error{errno, std::system_category(), "kevent64"};
}

ptrdiff_t kqueue_owner::events(const timespec& ts,
                               gsl::span<kevent64_s> events) noexcept(false) {
    // wait for events ...
    auto count = kevent64(kqfd, nullptr, 0,             //
                          events.data(), events.size(), //
                          0, &ts);
    if (count == -1)
        throw std::system_error{errno, std::system_category(), "kevent64"};
    return static_cast<ptrdiff_t>(count);
}

} // namespace coro
