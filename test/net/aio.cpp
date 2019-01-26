#include "net/async.h"
#include <vector>

#include <sys/event.h>

auto kqfd = kqueue();
constexpr auto max_event_size = 20;
auto events = std::make_unique<kevent64_s[]>(max_event_size);

extern void print_kevent(const kevent64_s& ev);

auto fetch_io_tasks() noexcept(false) -> enumerable<coroutine_task_t>
{
    assert(kqfd != -1);

    timespec* timeout = nullptr;
    // wait (indefinitely) for some event
    auto count = kevent64(kqfd,                         //
                          nullptr, 0,                   //
                          events.get(), max_event_size, //
                          0, timeout);

    assert(count != -1);
    std::printf("%s %d \n", __FUNCTION__, count);
    coroutine_task_t task{};
    for (auto i = 0; i < count; ++i)
    {
        auto& ev = events[i];
        print_kevent(ev);

        task = coroutine_task_t::from_address(
            // user data is coroutine frame in this case
            reinterpret_cast<void*>(ev.udata));

        co_yield task;
    }
}

bool io_work_t::ready() const noexcept
{
    return false;
}

uint32_t io_work_t::resume() noexcept
{
    return this->buffer.size();
}

auto send_to(int sd, const endpoint_t& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    work.socket = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(std::addressof(work));
}

void io_send_to::suspend(coroutine_task_t rh) noexcept(false)
{
    // system operation
    kevent64_s req{};
    req.ident = socket;
    req.filter = EVFILT_WRITE;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(rh.address());

    timespec* timeout = nullptr;
    // attach the event config
    auto ec = kevent64(kqfd,       //
                       &req, 1,    // change
                       nullptr, 0, //
                       0, timeout);
    assert(ec != -1);

    // user operation
    this->tag = rh.address();

    auto sz = sendto(socket,                                //
                     buffer.data(), buffer.size_bytes(), 0, //
                     reinterpret_cast<const sockaddr*>(to), sizeof(endpoint_t));

    if (sz == -1 && errno != EAGAIN)
        throw std::system_error{errno, std::system_category(), "sendto"};
}

auto recv_from(int sd, endpoint_t& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    work.socket = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;

    return *reinterpret_cast<io_recv_from*>(std::addressof(work));
}

void io_recv_from::suspend(coroutine_task_t rh) noexcept(false)
{
    // system operation
    kevent64_s req{};
    req.ident = socket;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(rh.address());

    timespec* timeout = nullptr;
    // attach the event config
    auto ec = kevent64(kqfd,       //
                       &req, 1,    // change
                       nullptr, 0, //
                       0, timeout);
    assert(ec != -1);

    // user operation
    this->tag = rh.address();

    auto sz = recvfrom(socket, buffer.data(), buffer.size_bytes(), 0,
                       reinterpret_cast<sockaddr*>(from),
                       std::addressof(this->length));

    if (sz == -1 && errno != EAGAIN)
        throw std::system_error{errno, std::system_category(), "recvfrom"};
}

void print_kevent(const kevent64_s& ev)
{
    printf(" ev.ident \t: %llu\n", ev.ident);
    printf(" ev.filter\t: %d\n", ev.filter);

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

    if (ev.fflags & EVFILT_READ)
        printf(" ev.fflags\t: EVFILT_READ\n");
    if (ev.fflags & EVFILT_EXCEPT)
        printf(" ev.fflags\t: EVFILT_EXCEPT\n");
    if (ev.fflags & EVFILT_WRITE)
        printf(" ev.fflags\t: EVFILT_WRITE\n");
    if (ev.fflags & EVFILT_AIO)
        printf(" ev.fflags\t: EVFILT_AIO\n");
    if (ev.fflags & EVFILT_VNODE)
        printf(" ev.fflags\t: EVFILT_VNODE\n");
    if (ev.fflags & EVFILT_PROC)
        printf(" ev.fflags\t: EVFILT_PROC\n");
    if (ev.fflags & EVFILT_SIGNAL)
        printf(" ev.fflags\t: EVFILT_SIGNAL\n");
    if (ev.fflags & EVFILT_MACHPORT)
        printf(" ev.fflags\t: EVFILT_MACHPORT\n");
    if (ev.fflags & EVFILT_TIMER)
        printf(" ev.fflags\t: EVFILT_TIMER\n");

    printf(" ev.data\t: %lld\n", ev.data);
    printf(" ev.udata\t: %llx, %llu\n", ev.udata, ev.udata);
}
