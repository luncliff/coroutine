// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>

#include <sys/event.h>
#include <unistd.h>

#include <gsl/gsl>

static_assert(sizeof(ssize_t) <= sizeof(int64_t));

// use 2 page for generator
const size_t max_event_size = 2 * getpagesize() / sizeof(kevent64_s);

auto kq_fd = kqueue();
auto kq_dtor = gsl::finally([]() noexcept { close(kq_fd); });
auto kq_events = std::make_unique<kevent64_s[]>(max_event_size);

auto fetch_io_tasks() noexcept(false) -> enumerable<coroutine_task_t>
{
    // wait (indefinitely) for events ...
    timespec* timeout{};

    auto count = kevent64(kq_fd, nullptr, 0,               //
                          kq_events.get(), max_event_size, //
                          0, timeout);
    if (count == -1)
        throw std::system_error{errno, std::system_category(), "kevent64"};

    for (auto i = 0; i < count; ++i)
    {
        auto& ev = kq_events[i];
        auto& work = *reinterpret_cast<io_work_t*>(ev.udata);
        // need to pass error information from
        // kevent to io_work
        co_yield work.task;
    }
}

bool io_work_t::ready() const noexcept
{
    // non blocking operation is expected
    // going to suspend
    if (fcntl(sd, F_GETFL, 0) & O_NONBLOCK)
        return false;

    // not configured. return true
    // and bypass to the blocking I/O
    return true;
}

auto send_to(int sd, const sockaddr_in& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    work.sd = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(std::addressof(work));
}

void io_send_to::suspend(coroutine_task_t rh) noexcept(false)
{
    this->task = rh;

    // system operation
    kevent64_s req{};
    req.ident = sd;
    req.filter = EVFILT_WRITE;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    auto ec = kevent64(kq_fd, &req, 1, // change
                       nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw std::system_error{errno, std::system_category(), "kevent64"};
}

int64_t io_send_to::resume() noexcept
{
    // now, available to send
    auto sz
        = sendto(sd, buffer.data(), buffer.size_bytes(), 0,
                 reinterpret_cast<const sockaddr*>(to), sizeof(sockaddr_in));
    // return follows that sendto
    return sz;
}
auto recv_from(int sd, sockaddr_in& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    work.sd = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;

    return *reinterpret_cast<io_recv_from*>(std::addressof(work));
}

void io_recv_from::suspend(coroutine_task_t rh) noexcept(false)
{
    this->task = rh;
    // system operation
    kevent64_s req{};
    req.ident = sd;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(static_cast<io_work_t*>(this));

    // attach the event config
    auto ec = kevent64(kq_fd, &req, 1, nullptr, 0, 0, nullptr);
    if (ec == -1)
        throw std::system_error{errno, std::system_category(), "kevent64"};
}

int64_t io_recv_from::resume() noexcept
{
    // recv event is detected
    socklen_t length = sizeof(sockaddr_in);
    auto sz = recvfrom(sd, buffer.data(), buffer.size_bytes(), //
                       0, reinterpret_cast<sockaddr*>(from),
                       std::addressof(length));
    // return follows that recvfrom
    return sz;
}

void print_kevent(const kevent64_s& ev)
{
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

char buf[NI_MAXHOST]{};
auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>
{
    ::gethostname(buf, NI_MAXHOST);
    return buf;
}

uint32_t nameof(const sockaddr_in6& ep, gsl::zstring<NI_MAXHOST> name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, nullptr,
                         0, flag);
}

uint32_t nameof(const sockaddr_in6& ep, //
                gsl::zstring<NI_MAXHOST> name,
                gsl::zstring<NI_MAXSERV> serv) noexcept
{
    //      NI_NAMEREQD
    //      NI_DGRAM
    //      NI_NOFQDN
    //      NI_NUMERICHOST
    //      NI_NUMERICSERV
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, serv,
                         NI_MAXSERV, flag);
}

auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept
    -> enumerable<sockaddr_in6>
{
    addrinfo* list = nullptr;

    if (auto ec = ::getaddrinfo(name, serv, //
                                std::addressof(hint), &list))
    {
        fputs(gai_strerror(ec), stderr);
        co_return;
    }

    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto d1 = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });

    for (addrinfo* iter = list; iter; iter = iter->ai_next)
    {
        sockaddr_in6& ep = *reinterpret_cast<sockaddr_in6*>(iter->ai_addr);
        // yield and proceed
        co_yield ep;
    }
}
