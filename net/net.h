// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  Note
//      Coroutine based network programming
//
//      Asynchronous Socket support with coroutine over Windows API
//       - Overlapped Callback
//       - I/O Completion Port (Proactor pattern)
//
//  To Do
//      More implementation + Test
//      - RFC 3540
//      - RFC 3168
//
// ---------------------------------------------------------------------------
#ifndef _MAGIC_NETWORK_IO_H_
#define _MAGIC_NETWORK_IO_H_

#include <magic/linkable.h>
#include <magic/coroutine.hpp>

#include <cstdint>
#include <cassert>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // System API

#include <WinSock2.h> // Windows Socket API
#include <ws2tcpip.h>
#include <ws2def.h>
#include <in6addr.h>

#pragma comment(lib, "Ws2_32.lib")

namespace magic
{
// - Note
//      Buffer is NOT a resource.
//      It just provide a view, without owning a memory.
//struct buffer final : public WSABUF
//{
//};
using buffer = WSABUF;
static_assert(sizeof(buffer) == sizeof(WSABUF));

// - Note
//      IP v6 address + port : 28 byte
// - Caution
//      Values are stored in Network Byte order.
//      Member functions support Host byte order
// - Reference
//      https://tools.ietf.org/html/rfc5952
//struct endpoint final
//{
//    uint16_t family;   // AF_INET6.
//    uint16_t port;	 // Transport level port number.
//    uint32_t flowinfo; // IPv6 flow information.
//    address  addr; // IPv6 address.
//    uint32_t scope_id; // Set of interfaces for a scope.
//};
using ip_address = ::in6_addr;
using endpoint = ::sockaddr_in6;
static_assert(sizeof(endpoint) == sizeof(::sockaddr_in6));

// - Note
//      System API structure for asynchronous I/O + library extension
// - See Also
//      `OVERLAPPED`
struct _INTERFACE_ io_work : public OVERLAPPED
{
    LPVOID tag{}; // User's custom data
    WSABUF *buffers{};
    DWORD count{};
    union {
        endpoint *from{};
        const endpoint *to;
    };

  public:
    constexpr bool ready() const noexcept { return false; }
    uint32_t resume() noexcept;

  protected:
    static void CALLBACK
    onWorkDone(DWORD dwError, DWORD dwBytes,
               LPWSAOVERLAPPED pover, DWORD flags) noexcept;
};
static_assert(sizeof(io_work) <= SYSTEM_CACHE_ALIGNMENT_SIZE);

class io_send_to;
class io_recv_from;
class io_send;
class io_recv;

_INTERFACE_
io_send_to &
send_to(SOCKET sd, const endpoint &remote, buffer *buf, uint32_t buflen,
        io_work &work) noexcept;

_INTERFACE_
io_recv_from &
recv_from(SOCKET sd, endpoint &remote, buffer *buf, uint32_t buflen,
          io_work &work) noexcept;

_INTERFACE_
io_recv &
recv(SOCKET sd, buffer *buf, uint32_t buflen,
     io_work &work) noexcept;

_INTERFACE_
io_send &
send(SOCKET sd, buffer *buf, uint32_t buflen,
     io_work &work) noexcept;

class _INTERFACE_ io_send_to final : public io_work
{
public:
    void suspend(stdex::coroutine_handle<> rh) noexcept;
};

class _INTERFACE_ io_recv_from final : public io_work
{
public:
    void suspend(stdex::coroutine_handle<> rh) noexcept;
};

class _INTERFACE_ io_send final : public io_work
{
public:
    void suspend(stdex::coroutine_handle<> rh) noexcept;
};

class _INTERFACE_ io_recv final : public io_work
{
public:
    void suspend(stdex::coroutine_handle<> rh) noexcept;
};

static_assert(sizeof(io_send_to) == sizeof(io_work));
static_assert(sizeof(io_recv_from) == sizeof(io_work));
static_assert(sizeof(io_send) == sizeof(io_work));
static_assert(sizeof(io_recv) == sizeof(io_work));

} // namespace magic

#endif // _MAGIC_NETWORK_IO_H_
