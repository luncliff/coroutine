//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/net.h>

using namespace std;
using namespace gsl;
namespace coro {

void wait_net_tasks(enumerable<io_task_t>& tasks,
                    chrono::nanoseconds) noexcept(false) {
    // windows implementation rely on callback.
    // So there is noting to yield ...
    tasks = enumerable<io_task_t>{};
    // Just comsume some items in this thread's APC queue
    SleepEx(0, true);
}

bool is_async_pending(int ec) noexcept {
    switch (ec) {
    case WSAEWOULDBLOCK:
    case EWOULDBLOCK:
    case EINPROGRESS:
    case ERROR_IO_PENDING:
        return true;
    default:
        return false;
    }
}

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(f .6)
void CALLBACK onWorkDone(DWORD errc, DWORD sz, LPWSAOVERLAPPED pover,
                         DWORD flags) noexcept {
    UNREFERENCED_PARAMETER(flags);
    io_work_t* work = reinterpret_cast<io_work_t*>(pover);

    // Mostly, `Internal` and `InternalHigh` holds exactly same value with the
    //  parameters. So these assignments are redundant.
    // Here, we are just making sure of it.
    work->Internal = errc;   // -> return of `work.error()`
    work->InternalHigh = sz; // -> return of `await_resume()`

    work->task.resume();
}

bool io_work_t::ready() const noexcept {
    return false; // always trigger `await_suspend` in Windows API
}

uint32_t get_io_error(const OVERLAPPED* target) noexcept {
    static_assert(sizeof(DWORD) == sizeof(uint32_t));
    return gsl::narrow_cast<uint32_t>(target->Internal);
}

int64_t get_io_length(const OVERLAPPED* target) noexcept {
    return gsl::narrow_cast<int64_t>(target->InternalHigh);
}

// see also: `onWorkDone`
uint32_t io_work_t::error() const noexcept {
    return get_io_error(this);
}

// zero memory the `OVERLAPPED` part in the `io_work_t`
auto zero_overlapped(gsl::not_null<io_control_block*> work) noexcept
    -> gsl::not_null<io_control_block*> {
    static_assert(is_same_v<io_control_block, OVERLAPPED>);

    *work = OVERLAPPED{};
    return work;
}

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(f .4) // for clang, this is not constexpr function.
auto make_wsa_buf(io_buffer_t v) noexcept -> WSABUF {
    WSABUF buf{}; // expect NRVO
    buf.buf = reinterpret_cast<char*>(v.data());
    buf.len = gsl::narrow_cast<ULONG>(v.size_bytes());
    return buf;
}

// ensure we are x64
static_assert(sizeof(SOCKET) == sizeof(uint64_t));
static_assert(sizeof(HANDLE) == sizeof(SOCKET));

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(type .3)
auto send_to(uint64_t sd, const sockaddr_in6& remote, io_buffer_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {

    work.hEvent = reinterpret_cast<HANDLE>(sd);
    work.Pointer = reinterpret_cast<sockaddr*>(
        const_cast<sockaddr_in6*>(addressof(remote)));
    work.buffer = buffer;
    work.Internal = DWORD{0}; // flag
    work.InternalHigh = sizeof(sockaddr_in6);

    // `co_await` operator will use reference to `io_send_to`
    return *reinterpret_cast<io_send_to*>(addressof(work));
}
GSL_SUPPRESS(type .1)
GSL_SUPPRESS(type .3)
auto send_to(uint64_t sd, const sockaddr_in& remote, io_buffer_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {

    work.hEvent = reinterpret_cast<HANDLE>(sd);
    work.Pointer = reinterpret_cast<sockaddr*>(
        const_cast<sockaddr_in*>(addressof(remote)));
    work.buffer = buffer;
    work.Internal = DWORD{0}; // flag
    work.InternalHigh = sizeof(sockaddr_in);

    // `co_await` operator will use reference to `io_send_to`
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(bounds .3)
void io_send_to::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = reinterpret_cast<SOCKET>(hEvent);
    const auto addr = reinterpret_cast<sockaddr*>(Pointer);
    const auto addrlen = gsl::narrow_cast<socklen_t>(InternalHigh);
    const auto flag = gsl::narrow_cast<DWORD>(Internal);
    WSABUF bufs[1] = {make_wsa_buf(buffer)};
    LPOVERLAPPED pover = zero_overlapped(gsl::make_not_null(this));

    if (::WSASendTo(sd, bufs, 1, nullptr, flag, //
                    addr, addrlen,              //
                    pover, onWorkDone) == NO_ERROR)
        return;

    if (const auto ec = WSAGetLastError()) {
        if (is_async_pending(ec))
            return;

        throw system_error{ec, system_category(), "WSASendTo"};
    }
}

int64_t io_send_to::resume() noexcept {
    return get_io_length(this);
}

GSL_SUPPRESS(type .1)
auto recv_from(uint64_t sd, sockaddr_in6& remote, io_buffer_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {

    work.hEvent = reinterpret_cast<HANDLE>(sd);
    work.Pointer = reinterpret_cast<sockaddr*>(addressof(remote));
    work.buffer = buffer;
    work.Internal = DWORD{0}; // flag
    work.InternalHigh = sizeof(sockaddr_in6);

    // `co_await` operator will use reference to `io_recv_from`
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

GSL_SUPPRESS(type .1)
auto recv_from(uint64_t sd, sockaddr_in& remote, io_buffer_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {

    work.hEvent = reinterpret_cast<HANDLE>(sd);
    work.Pointer = reinterpret_cast<sockaddr*>(addressof(remote));
    work.buffer = buffer;
    work.Internal = DWORD{0}; // flag
    work.InternalHigh = sizeof(sockaddr_in);

    // `co_await` operator will use reference to `io_recv_from`
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(bounds .3)
void io_recv_from::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = reinterpret_cast<SOCKET>(hEvent);
    const auto addr = reinterpret_cast<sockaddr*>(Pointer);
    auto addrlen = gsl::narrow_cast<socklen_t>(InternalHigh);
    auto flag = gsl::narrow_cast<DWORD>(Internal);
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    if (::WSARecvFrom(sd, bufs, 1, nullptr, &flag, //
                      addr, &addrlen,              //
                      zero_overlapped(gsl::make_not_null(this)),
                      onWorkDone) == NO_ERROR)
        return;

    if (const auto ec = WSAGetLastError()) {
        if (is_async_pending(ec))
            return;

        throw system_error{ec, system_category(), "WSARecvFrom"};
    }
}

int64_t io_recv_from::resume() noexcept {
    return get_io_length(this);
}

GSL_SUPPRESS(type .1)
auto send_stream(uint64_t sd, io_buffer_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_send& {

    work.hEvent = reinterpret_cast<HANDLE>(sd);
    work.buffer = buffer;
    work.Internal = flag;

    // `co_await` operator will use reference to `io_send`
    return *reinterpret_cast<io_send*>(addressof(work));
}

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(bounds .3)
void io_send::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = reinterpret_cast<SOCKET>(hEvent);
    const auto flag = gsl::narrow_cast<DWORD>(Internal);
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    if (::WSASend(sd, bufs, 1, nullptr, flag, //
                  zero_overlapped(gsl::make_not_null(this)),
                  onWorkDone) == NO_ERROR)
        return;

    if (const auto ec = WSAGetLastError()) {
        if (is_async_pending(ec))
            return;

        throw system_error{ec, system_category(), "WSASend"};
    }
}

int64_t io_send::resume() noexcept {
    return get_io_length(this);
}

GSL_SUPPRESS(type .1)
auto recv_stream(uint64_t sd, io_buffer_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_recv& {

    work.hEvent = reinterpret_cast<HANDLE>(sd);
    work.buffer = buffer;
    work.Internal = flag;

    // `co_await` operator will use reference to `io_recv`
    return *reinterpret_cast<io_recv*>(addressof(work));
}

GSL_SUPPRESS(type .1)
GSL_SUPPRESS(bounds .3)
void io_recv::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = reinterpret_cast<SOCKET>(hEvent);
    auto flag = gsl::narrow_cast<DWORD>(Internal);
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    if (::WSARecv(sd, bufs, 1, nullptr, &flag, //
                  zero_overlapped(gsl::make_not_null(this)),
                  onWorkDone) == NO_ERROR)
        return;

    if (const auto ec = WSAGetLastError()) {
        if (is_async_pending(ec))
            return;

        throw system_error{ec, system_category(), "WSARecv"};
    }
}

int64_t io_recv::resume() noexcept {
    return get_io_length(this);
}

} // namespace coro
