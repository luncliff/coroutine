// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
// #include <gsl/gsl_util>

using namespace std;
using namespace std::experimental;

uint32_t io_work::resume() noexcept
{
    return static_cast<uint32_t>(this->InternalHigh);
    // if (auto e = this->error()) {
    //    net::recent(e);
    //    return 0;
    //}
    // return this->bytes();
}

// GSL_SUPPRESS(type .1)
void CALLBACK io_work::onWorkDone( //
    DWORD dwError, DWORD dwBytes, LPWSAOVERLAPPED pover,
    DWORD flags) noexcept(false)
{
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(dwBytes);
    UNREFERENCED_PARAMETER(flags);

    io_work* work = reinterpret_cast<io_work*>(pover);
    assert(work->Internal == dwError);
    assert(work->InternalHigh == dwBytes);
    // if (dwError)
    //    work->Internal = dwError;
    // else
    //    work->InternalHigh = dwBytes;

    // `tag` is user custom data. For now, resumeable handle.
    coroutine_handle<void>::from_address(work->tag).resume();
}

// GSL_SUPPRESS(type .1)
auto send_to(SOCKET sd, const endpoint& remote, buffer* buf, uint32_t buflen,
             io_work& work) noexcept -> io_send_to&
{
    // start work construction

    // overlapped IO
    work.Pointer = reinterpret_cast<PVOID>(sd);

    // library custom
    work.tag = nullptr;
    work.buffers = buf;
    work.count = buflen;
    work.to = addressof(remote);

    return *reinterpret_cast<io_send_to*>(addressof(work));
}

// GSL_SUPPRESS(type .1)
void io_send_to::suspend(coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    const DWORD flag = 0;
    OVERLAPPED* block = this;

    // zero the memory
    *block = OVERLAPPED{};

    // `tag` is user custom data. For now, resumeable handle.
    this->tag = rh.address();

    // Overlapped Callback
    ::WSASendTo(sd, this->buffers, this->count, nullptr, flag,
                reinterpret_cast<const ::sockaddr*>(this->to),
                sizeof(::sockaddr_in6), block, onWorkDone);

    // IOCP
    //::WSASendTo(sd, const_cast<WSABUF *>(buffers), count,
    //    nullptr, flag,
    //    reinterpret_cast<const sockaddr *>(to), sizeof(sockaddr_in6),
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

// GSL_SUPPRESS(type .1)
auto recv_from(SOCKET sd, endpoint& remote, buffer* buf, uint32_t buflen,
               io_work& work) noexcept -> io_recv_from&
{
    // zero the memory + start work construction
    work = io_work{};

    // overlapped IO
    work.Pointer = reinterpret_cast<PVOID>(sd);

    // library custom
    work.tag = nullptr;
    work.buffers = buf;
    work.count = buflen;
    work.from = addressof(remote);

    return *reinterpret_cast<io_recv_from*>(addressof(work));
}

// GSL_SUPPRESS(type .1)
void io_recv_from::suspend(coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    DWORD flag = 0;
    int addrlen = sizeof(sockaddr_in6);
    OVERLAPPED* block = this;

    // zero the memory
    *block = OVERLAPPED{};

    this->tag = rh.address();

    // Overlapped Callback
    ::WSARecvFrom(sd, buffers, count, nullptr, &flag,
                  reinterpret_cast<sockaddr*>(this->from), &addrlen, block,
                  onWorkDone);

    //// IOCP
    //::WSARecvFrom(sd, buffers, count, nullptr, &flag,
    //    reinterpret_cast<sockaddr *>(from), &addrlen,
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

// GSL_SUPPRESS(type .1)
auto send(SOCKET sd, buffer* buf, uint32_t buflen, io_work& work) noexcept
    -> io_send&
{
    // zero the memory + start work construction
    work = io_work{};

    // overlapped IO
    work.Pointer = reinterpret_cast<PVOID>(sd);

    // library custom
    work.tag = nullptr;
    work.buffers = buf;
    work.count = buflen;
    work.to = nullptr;

    return *reinterpret_cast<io_send*>(addressof(work));
}

// GSL_SUPPRESS(type .1)
// GSL_SUPPRESS(type .3)
void io_send::suspend(coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    const DWORD flag = 0;
    OVERLAPPED* block = this;

    // zero the memory
    *block = OVERLAPPED{};

    // `tag` is user custom data. For now, resumeable handle.
    this->tag = rh.address();

    // Overlapped Callback
    ::WSASend(sd, const_cast<WSABUF*>(buffers), count, nullptr, flag, block,
              onWorkDone);

    //// IOCP
    //::WSASend(sd, const_cast<WSABUF *>(buffers), count,
    //    nullptr, flag,
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

// GSL_SUPPRESS(type .1)
auto recv(SOCKET sd, buffer* buf, uint32_t buflen, io_work& work) noexcept
    -> io_recv&
{
    // zero the memory + start work construction
    work = io_work{};

    // overlapped IO
    work.Pointer = reinterpret_cast<PVOID>(sd);

    // library custom
    work.tag = nullptr;
    work.buffers = buf;
    work.count = buflen;
    work.to = nullptr;

    return *reinterpret_cast<io_recv*>(addressof(work));
}

// GSL_SUPPRESS(type .1)
void io_recv::suspend(coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    DWORD flag = 0;
    OVERLAPPED* block = this;

    // zero the memory
    *block = OVERLAPPED{};

    this->tag = rh.address();

    // Overlapped Callback
    ::WSARecv(sd, buffers, count, nullptr, &flag, block, onWorkDone);

    //// IOCP
    //::WSARecv(sd,
    //    buffers, count,
    //    nullptr, &flag,
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}
