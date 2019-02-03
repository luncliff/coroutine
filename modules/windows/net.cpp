// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>

void CALLBACK onWorkDone(DWORD errc, DWORD sz, LPWSAOVERLAPPED pover,
                         DWORD flags) noexcept
{
    UNREFERENCED_PARAMETER(flags);

    io_work_t* work = reinterpret_cast<io_work_t*>(pover);
    // this is expected value for x64
    assert(work->Internal == errc);
    assert(work->InternalHigh == sz);
    work->task.resume();
}

bool io_work_t::ready() const noexcept
{
    return false; // trigger await_suspend
}

auto send_to(uint64_t sd, const sockaddr_in6& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    static_assert(sizeof(SOCKET) == sizeof(uint64_t));
    static_assert(sizeof(HANDLE) == sizeof(SOCKET));

    // start i/o request construction.
    work.buffer = buffer;
    work.to6 = std::addressof(remote);

    // forward the other args using OVERLAPPED
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);

    // lead to co_await operations with `io_send_to` type
    return *reinterpret_cast<io_send_to*>(std::addressof(work));
}

auto send_to(uint64_t sd, const sockaddr_in& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    static_assert(sizeof(SOCKET) == sizeof(uint64_t));
    static_assert(sizeof(HANDLE) == sizeof(SOCKET));

    // start i/o request construction.
    work.buffer = buffer;
    work.to = std::addressof(remote);

    // forward the other args using OVERLAPPED
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);

    // lead to co_await operations with `io_send_to` type
    return *reinterpret_cast<io_send_to*>(std::addressof(work));
}

void io_send_to::suspend(coroutine_task_t rh) noexcept(false)
{
    const auto addrlen = static_cast<socklen_t>(this->InternalHigh);
    const auto flag = DWORD{0};
    // socket must be known before zero the overlapped
    const auto sd = static_cast<SOCKET>(this->Internal);
    WSABUF buffers[1]{};

    // for now, scatter-gather is not supported
    buffers[0].buf = reinterpret_cast<char*>(this->buffer.data());
    buffers[0].len = static_cast<ULONG>(this->buffer.size_bytes());

    // coroutine frame for the i/o callback
    this->task = rh;

    // zero the memory
    OVERLAPPED* pover = this;
    *pover = OVERLAPPED{};

    // rely on Overlapped Callback
    //  in case of IOCP, callback must be null
    ::WSASendTo(sd, buffers, 1, nullptr, flag, //
                this->ep, addrlen, pover, onWorkDone);

    const auto ec = WSAGetLastError();
    if (ec == NO_ERROR || ec == ERROR_IO_PENDING)
        return; // ok. expected for async i/o

    throw std::system_error{ec, std::system_category(), "WSASendTo"};
}

int64_t io_send_to::resume() noexcept
{
    // need error handling
    return static_cast<uint32_t>(this->InternalHigh);
}

auto recv_from(uint64_t sd, sockaddr_in6& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    static_assert(sizeof(SOCKET) == sizeof(uint64_t));
    static_assert(sizeof(HANDLE) == sizeof(SOCKET));

    // start i/o request construction.
    work.buffer = buffer;
    work.from6 = std::addressof(remote);

    // forward the other args using OVERLAPPED
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);

    // lead to co_await operations with `io_recv_from` type
    return *reinterpret_cast<io_recv_from*>(std::addressof(work));
}

auto recv_from(uint64_t sd, sockaddr_in& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    static_assert(sizeof(SOCKET) == sizeof(uint64_t));
    static_assert(sizeof(HANDLE) == sizeof(SOCKET));

    // start i/o request construction.
    work.buffer = buffer;
    work.from = std::addressof(remote);

    // forward the other args using OVERLAPPED
    work.Internal = sd;
    work.InternalHigh = sizeof(remote);

    // lead to co_await operations with `io_recv_from` type
    return *reinterpret_cast<io_recv_from*>(std::addressof(work));
}

void io_recv_from::suspend(coroutine_task_t rh) noexcept(false)
{
    auto addrlen = static_cast<socklen_t>(this->InternalHigh);
    auto flag = DWORD{0};
    // socket must be known before zero the overlapped
    const auto sd = static_cast<SOCKET>(this->Internal);
    WSABUF buffers[1]{};

    // for now, scatter-gather is not supported
    buffers[0].buf = reinterpret_cast<char*>(this->buffer.data());
    buffers[0].len = static_cast<ULONG>(this->buffer.size_bytes());

    // coroutine frame for the i/o callback
    task = rh;

    OVERLAPPED* pover = this;
    *pover = OVERLAPPED{}; // zero the memory

    // rely on Overlapped Callback
    //  in case of IOCP, callback must be null
    ::WSARecvFrom(sd, buffers, 1, nullptr, &flag, //
                  this->ep, &addrlen, pover, onWorkDone);

    const auto ec = WSAGetLastError();
    if (ec == NO_ERROR || ec == ERROR_IO_PENDING)
        return; // ok. expected for async i/o

    throw std::system_error{ec, std::system_category(), "WSARecvFrom"};
}

int64_t io_recv_from::resume() noexcept
{
    // need error handling
    return static_cast<uint32_t>(this->InternalHigh);
}

class wsa_api_data : public WSADATA
{
  public:
    wsa_api_data() noexcept(false)
    {
        ::WSAStartup(MAKEWORD(2, 2), this);
    }
    ~wsa_api_data() noexcept
    {
        ::WSACleanup();
    }
};

wsa_api_data trigger_winsock_load{};
