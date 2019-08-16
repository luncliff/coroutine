//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "kernel_queue.h"

#include <system_error>
using namespace std;
namespace coro {

kernel_queue_t::kernel_queue_t() noexcept(false)
    : kqfd{-1},
      // use 2 page for polling
      capacity{2 * getpagesize() / sizeof(kevent64_s)},
      events{make_unique<kevent64_s[]>(capacity)} {
    kqfd = kqueue();
    if (kqfd < 0)
        throw system_error{errno, system_category(), "kqueue"};
}
kernel_queue_t::~kernel_queue_t() noexcept {
    close(kqfd);
}
void kernel_queue_t::change(kevent64_s& req) noexcept(false) {
    // attach the event config
    auto ec = kevent64(kqfd, &req, 1, nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw system_error{errno, system_category(), "kevent64"};
}
auto kernel_queue_t::wait(const timespec& ts) noexcept(false)
    -> enumerable<kevent64_s> {
    // wait for events ...
    auto count = kevent64(kqfd, nullptr, 0,       //
                          events.get(), capacity, //
                          0, &ts);
    if (count == -1)
        throw system_error{errno, system_category(), "kevent64"};

    for (auto i = 0; i < count; ++i) {
        co_yield events[i];
    }
}
} // namespace coro

/*
void print_kevent(const kevent64_s& ev) {
    printf(" ev.ident \t: %llu\n", ev.ident);

    if (ev.filter == EVFILT_READ)
        printf(" ev.filter\t: EVFILT_READ\n");
    if (ev.filter == EVFILT_WRITE)
        printf(" ev.filter\t: EVFILT_WRITE\n");
    if (ev.filter == EVFILT_EXCEPT)
        printf(" ev.filter\t: EVFILT_EXCEPT\n");
    if (ev.filter == EVFILT_AIO)
        printf(" ev.filter\t: EVFILT_AIO\n");
    if (ev.filter == EVFILT_VNODE)
        printf(" ev.filter\t: EVFILT_VNODE\n");
    if (ev.filter == EVFILT_PROC)
        printf(" ev.filter\t: EVFILT_PROC\n");
    if (ev.filter == EVFILT_SIGNAL)
        printf(" ev.filter\t: EVFILT_SIGNAL\n");
    if (ev.filter == EVFILT_MACHPORT)
        printf(" ev.filter\t: EVFILT_MACHPORT\n");
    if (ev.filter == EVFILT_TIMER)
        printf(" ev.filter\t: EVFILT_TIMER\n");

    if (ev.flags & EV_ADD)
        printf(" ev.flags\t: EV_ADD\n");
    if (ev.flags & EV_DELETE)
        printf(" ev.flags\t: EV_DELETE\n");
    if (ev.flags & EV_ENABLE)
        printf(" ev.flags\t: EV_ENABLE\n");
    if (ev.flags & EV_DISABLE)
        printf(" ev.flags\t: EV_DISABLE\n");
    if (ev.flags & EV_ONESHOT)
        printf(" ev.flags\t: EV_ONESHOT\n");
    if (ev.flags & EV_CLEAR)
        printf(" ev.flags\t: EV_CLEAR\n");
    if (ev.flags & EV_RECEIPT)
        printf(" ev.flags\t: EV_RECEIPT\n");
    if (ev.flags & EV_DISPATCH)
        printf(" ev.flags\t: EV_DISPATCH\n");
    if (ev.flags & EV_UDATA_SPECIFIC)
        printf(" ev.flags\t: EV_UDATA_SPECIFIC\n");
    if (ev.flags & EV_VANISHED)
        printf(" ev.flags\t: EV_VANISHED\n");
    if (ev.flags & EV_ERROR)
        printf(" ev.flags\t: EV_ERROR\n");
    if (ev.flags & EV_OOBAND)
        printf(" ev.flags\t: EV_OOBAND\n");
    if (ev.flags & EV_EOF)
        printf(" ev.flags\t: EV_EOF\n");

    printf(" ev.data\t: %lld\n", ev.data);
    printf(" ev.udata\t: %llx, %llu\n", ev.udata, ev.udata);
}
*/
