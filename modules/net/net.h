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
#ifndef LINKABLE_DLL_MACRO
#define LINKABLE_DLL_MACRO

#ifdef _MSC_VER // MSVC
#define _HIDDEN_
#ifdef _WINDLL
#define _INTERFACE_ __declspec(dllexport)
#else
#define _INTERFACE_ __declspec(dllimport)
#endif

#elif __GNUC__ || __clang__ // GCC or Clang
#define _INTERFACE_ __attribute__((visibility("default")))
#define _HIDDEN_ __attribute__((visibility("hidden")))

#else
#error "unexpected compiler"

#endif // compiler check
#endif // LINKABLE_DLL_MACRO

#ifndef COROUTINE_NETWORK_IO_H
#define COROUTINE_NETWORK_IO_H

#include <cassert>
#include <numeric>

#include <experimental/coroutine>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // System API

#include <WinSock2.h> // Windows Socket API
#include <in6addr.h>
#include <ws2def.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

// - Note
//      Buffer is NOT a resource.
//      It just provide a view, without owning a memory.
// struct buffer final : public WSABUF
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
// struct endpoint final
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
    WSABUF* buffers{};
    DWORD count{};
    union {
        endpoint* from{};
        const endpoint* to;
    };

  public:
    constexpr bool ready() const noexcept { return false; }
    uint32_t resume() noexcept;

  protected:
    static void CALLBACK //
    onWorkDone(DWORD dwError,
               DWORD dwBytes,
               LPWSAOVERLAPPED pover,
               DWORD flags) noexcept;
};
static_assert(sizeof(io_work) <= SYSTEM_CACHE_ALIGNMENT_SIZE);

class io_send_to;
class io_recv_from;
class io_send;
class io_recv;

[[nodiscard]] _INTERFACE_ //
    io_send_to&
    send_to(SOCKET sd,
            const endpoint& remote,
            buffer* buf,
            uint32_t buflen,
            io_work& work) noexcept;

[[nodiscard]] _INTERFACE_ //
    io_recv_from&
    recv_from(SOCKET sd,
              endpoint& remote,
              buffer* buf,
              uint32_t buflen,
              io_work& work) noexcept;

[[nodiscard]] _INTERFACE_ //
    io_recv&
    recv(SOCKET sd, buffer* buf, uint32_t buflen, io_work& work) noexcept;

[[nodiscard]] _INTERFACE_ //
    io_send&
    send(SOCKET sd, buffer* buf, uint32_t buflen, io_work& work) noexcept;

class _INTERFACE_ io_send_to final : public io_work
{
  public:
    void suspend(std::experimental::coroutine_handle<> rh) noexcept;
};

class _INTERFACE_ io_recv_from final : public io_work
{
  public:
    void suspend(std::experimental::coroutine_handle<> rh) noexcept;
};

class _INTERFACE_ io_send final : public io_work
{
  public:
    void suspend(std::experimental::coroutine_handle<> rh) noexcept;
};

class _INTERFACE_ io_recv final : public io_work
{
  public:
    void suspend(std::experimental::coroutine_handle<> rh) noexcept;
};

static_assert(sizeof(io_send_to) == sizeof(io_work));
static_assert(sizeof(io_recv_from) == sizeof(io_work));
static_assert(sizeof(io_send) == sizeof(io_work));
static_assert(sizeof(io_recv) == sizeof(io_work));

#pragma warning(disable : 4505)

static decltype(auto) //
await_ready(const io_send_to& a) noexcept
{
    return a.ready();
}
static decltype(auto) //
await_suspend(io_send_to& a,
              std::experimental::coroutine_handle<void> rh) noexcept(false)
{
    return a.suspend(rh);
}
static decltype(auto) //
await_resume(io_send_to& a) noexcept
{
    return a.resume();
}

static decltype(auto) //
await_ready(const io_recv_from& a) noexcept
{
    return a.ready();
}
static decltype(auto) //
await_suspend(io_recv_from& a,
              std::experimental::coroutine_handle<void> rh) noexcept(false)
{
    return a.suspend(rh);
}
static decltype(auto) //
await_resume(io_recv_from& a) noexcept
{
    return a.resume();
}

static decltype(auto) //
await_ready(const io_send& a) noexcept
{
    return a.ready();
}
static decltype(auto) //
await_suspend(io_send& a,
              std::experimental::coroutine_handle<void> rh) noexcept(false)
{
    return a.suspend(rh);
}
static decltype(auto) //
await_resume(io_send& a) noexcept
{
    return a.resume();
}

static decltype(auto) //
await_ready(const io_recv& a) noexcept
{
    return a.ready();
}
static decltype(auto) //
await_suspend(io_recv& a,
              std::experimental::coroutine_handle<void> rh) noexcept(false)
{
    return a.suspend(rh);
}
static decltype(auto) //
await_resume(io_recv& a) noexcept
{
    return a.resume();
}

#pragma warning(default : 4505)

#endif // COROUTINE_NETWORK_IO_H
