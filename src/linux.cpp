/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/linux.h>

#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace std;

namespace coro {

epoll_owner::epoll_owner() noexcept(false)
    : epfd{epoll_create1(EPOLL_CLOEXEC)} {
    if (epfd < 0)
        throw system_error{errno, system_category(), "epoll_create1"};
}
epoll_owner::~epoll_owner() noexcept {
    close(epfd);
}

void epoll_owner::try_add(uint64_t fd, epoll_event& req) noexcept(false) {
    int op = EPOLL_CTL_ADD, ec = 0;
TRY_OP:
    ec = epoll_ctl(epfd, op, fd, &req);
    if (ec == 0)
        return;
    if (errno == EEXIST) {
        op = EPOLL_CTL_MOD; // already exists. try with modification
        goto TRY_OP;
    }

    throw system_error{errno, system_category(),
                       "epoll_ctl(EPOLL_CTL_ADD|EPOLL_CTL_MODE)"};
}

void epoll_owner::remove(uint64_t fd) {
    epoll_event req{}; // just prevent non-null input
    const auto ec = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &req);
    if (ec != 0)
        throw system_error{errno, system_category(),
                           "epoll_ctl(EPOLL_CTL_DEL)"};
}
ptrdiff_t epoll_owner::wait(uint32_t wait_ms,
                            gsl::span<epoll_event> output) noexcept(false) {
    auto count = epoll_wait(epfd, output.data(), output.size(), wait_ms);
    if (count == -1)
        throw system_error{errno, system_category(), "epoll_wait"};
    return count;
}

//
//  We are going to combine file descriptor and state bit
//
//  On x86 system,
//    this won't matter since `int` is 32 bit.
//    we can safely use msb for state indicator.
//
//  On x64 system,
//    this might be a hazardous since the value of `eventfd` can be corrupted.
//    **Normally** descriptor in Linux system grows from 3, so it is highly
//    possible to reach system limitation before the value consumes all 63 bit.
//
constexpr uint64_t emask = 1ULL << 63;

//  the msb(most significant bit) will be ...
//   1 if the fd is signaled,
//   0 on the other case
bool is_signaled(uint64_t state) noexcept {
    return emask & state; // msb is 1?
}

int64_t get_eventfd(uint64_t state) noexcept {
    return static_cast<int64_t>(~emask & state);
}

void notify_event(int64_t efd) noexcept(false) {
    // signal the eventfd...
    //  the message can be any value
    //  since the purpose of it is to trigger the epoll
    //  we won't care about the internal counter of the eventfd
    if (write(efd, &efd, sizeof(efd)) == -1)
        throw system_error{errno, system_category(), "write"};
}

void consume_event(int64_t efd) noexcept(false) {
    if (read(efd, &efd, sizeof(efd)) == -1)
        throw system_error{errno, system_category(), "read"};
}

event::event() noexcept(false) : state{} {
    const auto fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1)
        throw system_error{errno, system_category(), "eventfd"};

    this->state = fd; // start with unsignaled state
}
event::~event() noexcept {
    // if already closed, fd == 0
    if (auto fd = get_eventfd(state))
        close(fd);
}

uint64_t event::fd() const noexcept {
    return get_eventfd(state);
}

bool event::is_set() const noexcept {
    return is_signaled(state);
}

void event::set() noexcept(false) {
    // already signaled. nothing to do...
    if (is_signaled(state))
        // !!! under the race condition, this check is not safe !!!
        return;

    auto fd = get_eventfd(state);
    notify_event(fd);                          // if it didn't throwed
    state = emask | static_cast<uint64_t>(fd); //  it's signaled state from now
}

void event::reset() noexcept(false) {
    const auto fd = get_eventfd(state);
    // if already signaled. nothing to do...
    if (is_signaled(state))
        consume_event(fd);
    // make unsignaled state
    this->state = static_cast<uint64_t>(fd);
}

} // namespace coro
