// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  Reference
//      - https://msdn.microsoft.com/en-us/library/windows/desktop/ms684124(v=vs.85).aspx
//
// ---------------------------------------------------------------------------
#include "./net.h"

namespace coroutine
{
uint32_t io_work::resume() noexcept
{
    return static_cast<uint32_t>(this->InternalHigh);
    //if (auto e = this->error()) {
    //    net::recent(e);
    //    return 0;
    //}
    //return this->bytes();
}

void CALLBACK
io_work::onWorkDone(DWORD dwError, DWORD dwBytes,
                    LPWSAOVERLAPPED pover, DWORD flags) noexcept
{
    UNREFERENCED_PARAMETER(flags);

    io_work *work = reinterpret_cast<io_work *>(pover);
    assert(work->Internal == dwError);
    assert(work->InternalHigh == dwBytes);
    //if (dwError)
    //    work->Internal = dwError;
    //else
    //    work->InternalHigh = dwBytes;

    // `tag` is user custom data. For now, resumeable handle.
    return std::experimental::coroutine_handle<>::from_address(work->tag).resume();
}

auto send_to(SOCKET sd, const endpoint &remote,
             buffer *buf, uint32_t buflen,
             io_work &work) noexcept
    -> io_send_to &
{
    // start work construction

    // overlapped IO
    work.Pointer = reinterpret_cast<PVOID>(sd);

    // library custom
    work.tag = nullptr;
    work.buffers = buf;
    work.count = buflen;
    work.to = std::addressof(remote);

    return *reinterpret_cast<io_send_to *>(std::addressof(work));
}

void io_send_to::suspend(std::experimental::coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    const DWORD flag = 0;
    OVERLAPPED *block = this;

    // zero the memory
    *block = OVERLAPPED{};

    // `tag` is user custom data. For now, resumeable handle.
    this->tag = rh.address();

    // Overlapped Callback
    ::WSASendTo(sd,
                this->buffers, this->count,
                nullptr, flag,
                reinterpret_cast<const ::sockaddr *>(this->to),
                sizeof(::sockaddr_in6),
                block, onWorkDone);

    // IOCP
    //::WSASendTo(sd, const_cast<WSABUF *>(buffers), count,
    //    nullptr, flag,
    //    reinterpret_cast<const sockaddr *>(to), sizeof(sockaddr_in6),
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

auto recv_from(SOCKET sd, endpoint &remote,
               buffer *buf, uint32_t buflen,
               io_work &work) noexcept -> io_recv_from &
{
    // zero the memory + start work construction
    work = io_work{};

    // overlapped IO
    work.Pointer = reinterpret_cast<PVOID>(sd);

    // library custom
    work.tag = nullptr;
    work.buffers = buf;
    work.count = buflen;
    work.from = std::addressof(remote);

    return *reinterpret_cast<io_recv_from *>(std::addressof(work));
}

void io_recv_from::suspend(std::experimental::coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    DWORD flag = 0;
    int addrlen = sizeof(sockaddr_in6);
    OVERLAPPED *block = this;

    // zero the memory
    *block = OVERLAPPED{};

    this->tag = rh.address();

    // Overlapped Callback
    ::WSARecvFrom(sd, buffers, count, nullptr, &flag,
                  reinterpret_cast<sockaddr *>(this->from), &addrlen,
                  block, onWorkDone);

    //// IOCP
    //::WSARecvFrom(sd, buffers, count, nullptr, &flag,
    //    reinterpret_cast<sockaddr *>(from), &addrlen,
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

auto send(SOCKET sd,
          buffer *buf, uint32_t buflen,
          io_work &work) noexcept -> io_send &
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

    return *reinterpret_cast<io_send *>(std::addressof(work));
}

void io_send::suspend(std::experimental::coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    const DWORD flag = 0;
    OVERLAPPED *block = this;

    // zero the memory
    *block = OVERLAPPED{};

    // `tag` is user custom data. For now, resumeable handle.
    this->tag = rh.address();

    // Overlapped Callback
    ::WSASend(sd, const_cast<WSABUF *>(buffers), count,
              nullptr, flag,
              block, onWorkDone);

    //// IOCP
    //::WSASend(sd, const_cast<WSABUF *>(buffers), count,
    //    nullptr, flag,
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

auto recv(SOCKET sd,
          buffer *buf, uint32_t buflen,
          io_work &work) noexcept -> io_recv &
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

    return *reinterpret_cast<io_recv *>(std::addressof(work));
}

void io_recv::suspend(std::experimental::coroutine_handle<> rh) noexcept
{
    const SOCKET sd = reinterpret_cast<SOCKET>(this->Pointer);
    DWORD flag = 0;
    OVERLAPPED *block = this;

    // zero the memory
    *block = OVERLAPPED{};

    this->tag = rh.address();

    // Overlapped Callback
    ::WSARecv(sd,
              buffers, count,
              nullptr, &flag,
              block, onWorkDone);

    //// IOCP
    //::WSARecv(sd,
    //    buffers, count,
    //    nullptr, &flag,
    //    block, nullptr);

    const auto ec = WSAGetLastError();
    assert(ec == S_OK || ec == ERROR_IO_PENDING);
}

} // namespace magic
