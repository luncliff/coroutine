// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>

using namespace std;
using namespace gsl;

// throws `system_error` if `WSAGetLastError` returns error code
void throw_if_async_error(not_null<czstring<>> label) noexcept(false) {
    const auto ec = WSAGetLastError();
    if (ec == NO_ERROR || ec == ERROR_IO_PENDING)
        return; // ok. expected for async i/o

    throw system_error{ec, system_category(), label};
}

GSL_SUPPRESS(es .76)
GSL_SUPPRESS(gsl.util)
auto wait_io_tasks(chrono::nanoseconds) noexcept(false)
    -> coro::enumerable<io_task_t> {
    // windows implementation rely on callback.
    // So this function will yield nothing
    co_return;
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
    return false; // always trigger `await_suspend` in windows API
}

// see also: `onWorkDone`
uint32_t io_work_t::error() const noexcept {
    static_assert(sizeof(DWORD) == sizeof(uint32_t));
    return gsl::narrow_cast<uint32_t>(Internal);
}

// zero memory the `OVERLAPPED` part in the `io_work_t`
auto zero_overlapped(gsl::not_null<io_work_t*> work) noexcept
    -> gsl::not_null<OVERLAPPED*> {
    OVERLAPPED* ptr = work;
    *ptr = OVERLAPPED{};
    return ptr;
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

    work.buffer = buffer;
    work.ep = reinterpret_cast<endpoint_t*>(
        const_cast<sockaddr_in6*>(addressof(remote)));
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);
    // lead `co_await` operator to `io_send_to` type
    return *reinterpret_cast<io_send_to*>(addressof(work));
}
GSL_SUPPRESS(type .1)
GSL_SUPPRESS(type .3)
auto send_to(uint64_t sd, const sockaddr_in& remote, io_buffer_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to& {

    work.buffer = buffer;
    work.ep = reinterpret_cast<endpoint_t*>(
        const_cast<sockaddr_in*>(addressof(remote)));
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);
    // lead `co_await` operator to `io_send_to` type
    return *reinterpret_cast<io_send_to*>(addressof(work));
}

GSL_SUPPRESS(bounds .3)
void io_send_to::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = gsl::narrow_cast<SOCKET>(Internal);
    const auto addr = addressof(this->ep->addr);
    const auto addrlen = gsl::narrow_cast<socklen_t>(InternalHigh);
    const auto flag = DWORD{0};
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    ::WSASendTo(sd, bufs, 1, nullptr, flag, addr, addrlen,
                zero_overlapped(this), onWorkDone);
    throw_if_async_error("WSASendTo");
}

int64_t io_send_to::resume() noexcept {
    return gsl::narrow_cast<int64_t>(InternalHigh);
}

GSL_SUPPRESS(type .1)
auto recv_from(uint64_t sd, sockaddr_in6& remote, io_buffer_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {

    work.buffer = buffer;
    work.ep = reinterpret_cast<endpoint_t*>(addressof(remote));
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);
    // lead `co_await` operator to `io_recv_from` type
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

GSL_SUPPRESS(type .1)
auto recv_from(uint64_t sd, sockaddr_in& remote, io_buffer_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from& {

    work.buffer = buffer;
    work.ep = reinterpret_cast<endpoint_t*>(addressof(remote));
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);
    // lead `co_await` operator to `io_recv_from` type
    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

GSL_SUPPRESS(bounds .3)
void io_recv_from::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = gsl::narrow_cast<SOCKET>(Internal);
    auto addr = addressof(this->ep->addr);
    auto addrlen = gsl::narrow_cast<socklen_t>(InternalHigh);
    auto flag = DWORD{0};
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    ::WSARecvFrom(sd, bufs, 1, nullptr, &flag, addr, &addrlen,
                  zero_overlapped(this), onWorkDone);
    throw_if_async_error("WSARecvFrom");
}

int64_t io_recv_from::resume() noexcept {
    return gsl::narrow_cast<int64_t>(InternalHigh);
}

GSL_SUPPRESS(type .1)
auto send_stream(uint64_t sd, io_buffer_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_send& {

    work.buffer = buffer;
    work.Internal = sd;
    work.InternalHigh = flag;
    // lead `co_await` operator to `io_send` type
    return *reinterpret_cast<io_send*>(addressof(work));
}

GSL_SUPPRESS(bounds .3)
void io_send::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = gsl::narrow_cast<SOCKET>(Internal);
    const auto flag = gsl::narrow_cast<DWORD>(InternalHigh);
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    ::WSASend(sd, bufs, 1, nullptr, flag, zero_overlapped(this), onWorkDone);
    throw_if_async_error("WSASend");
}

int64_t io_send::resume() noexcept {
    return gsl::narrow_cast<int64_t>(InternalHigh);
}

GSL_SUPPRESS(type .1)
auto recv_stream(uint64_t sd, io_buffer_t buffer, uint32_t flag,
                 io_work_t& work) noexcept(false) -> io_recv& {

    work.buffer = buffer;
    work.Internal = sd;
    work.InternalHigh = flag;
    // lead `co_await` operator to `io_recv` type
    return *reinterpret_cast<io_recv*>(addressof(work));
}

GSL_SUPPRESS(bounds .3)
void io_recv::suspend(io_task_t t) noexcept(false) {
    task = t; // coroutine will be resumed in overlapped callback

    const auto sd = gsl::narrow_cast<SOCKET>(Internal);
    auto flag = gsl::narrow_cast<DWORD>(InternalHigh);
    WSABUF bufs[1] = {make_wsa_buf(buffer)};

    ::WSARecv(sd, bufs, 1, nullptr, &flag, zero_overlapped(this), onWorkDone);
    throw_if_async_error("WSARecv");
}

int64_t io_recv::resume() noexcept {
    return gsl::narrow_cast<int64_t>(InternalHigh);
}
