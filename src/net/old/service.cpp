// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  Reference
//		-
//
// ---------------------------------------------------------------------------
#include "./service.h"

namespace net
{
service::service(uint16_t max) noexcept : cp{max}
{
}

error service::poll(uint32_t timeout) noexcept
{
    io_work *work = cp.get(timeout);
    if (!work)
        return recent();

    stdex::coroutine_handle<>::from_address(work->tag).resume();
    return error{};
}

SOCKET
service::open(category type, proto protocol) noexcept
{
    SOCKET sd = ::WSASocketW(AF_INET6, (int)type, (int)protocol,
                             nullptr, 0, WSA_FLAG_OVERLAPPED);

    if (sd != INVALID_SOCKET)
        this->bind(sd); // IOCP Binding
    return sd;
}

SOCKET
service::dial(const endpoint &remote) noexcept
{
    SOCKET sd = tcp::dial(remote);
    if (sd != INVALID_SOCKET)
        this->bind(sd); // IOCP Binding
    return sd;
}

SOCKET
service::listen(const endpoint &local,
                int backlog) noexcept
{
    DWORD group = 0;
    WSAPROTOCOL_INFOW *info = nullptr;

    // TCP listener socket
    SOCKET sd = ::WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
                             info, group, WSA_FLAG_OVERLAPPED);
    if (sd == INVALID_SOCKET)
        goto Error;
    // IOCP Binding
    if (cp.bind((HANDLE)sd, sd))
        goto Error;
    if (::bind(sd, reinterpret_cast<const sockaddr *>(&local),
               sizeof(sockaddr_in6)) == SOCKET_ERROR)
        goto Error;
    if (::listen(sd, backlog) == SOCKET_ERROR)
        goto Error;

    // Sucess
    return sd;
Error:
    error ec = recent();
    ::closesocket(sd);   // Close anyway
    WSASetLastError(ec); // Recover error code
    return INVALID_SOCKET;
}

error service::bind(SOCKET sd) noexcept
{
    // IOCP Binding
    return cp.bind((HANDLE)sd, sd);
}

} // namespace net

/*
iocp::iocp(uint32_t max) noexcept
{
    cport = ::CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, nullptr, 0, max);
}

iocp::~iocp() noexcept
{
    ::CloseHandle(cport);
}

uint32_t iocp::bind(HANDLE h, uint64_t _key) noexcept
{
    ULONG_PTR io_key = static_cast<ULONG_PTR>(_key);
    cport = ::CreateIoCompletionPort(h, cport, io_key, 0);
    return GetLastError();
}

uint32_t iocp::post(io_work &cb) noexcept
{
    ULONG_PTR key = cb.key();
    ::PostQueuedCompletionStatus(cport, cb.bytes(), key, &cb);
    return ::GetLastError();
}

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa364986(v=vs.85).aspx
io_work *iocp::get(uint32_t timeout) noexcept
{
    io_work *work = nullptr; // OVERLAPPED*
    DWORD dwBytes{};
    ULONG_PTR ulKey{};
    // Dispatched I/O completion packet.
    if (::GetQueuedCompletionStatus(
            cport, &dwBytes, &ulKey,
            reinterpret_cast<LPOVERLAPPED *>(&work),
            timeout))
    {
        work->key(ulKey);
        work->bytes(dwBytes);
    }
    // Failed : I/O
    else if (work)
    {
        work->key(ulKey);
        work->error(::GetLastError());
    }
    // Failed : `GetQueuedCompletionStatus`
    else
    {
        // In this case, work is nullptr
    }
    return work;
}

iocp::operator bool() const noexcept
{
    return cport != NULL;
}
*/
