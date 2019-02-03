// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Async I/O operation support for socket
//
// ---------------------------------------------------------------------------
#pragma once
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

#ifndef COROUTINE_NET_IO_H
#define COROUTINE_NET_IO_H

#include <coroutine/enumerable.hpp>

#include <gsl/gsl>

#ifdef _MSC_VER
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <ws2def.h>

#pragma comment(lib, "Ws2_32.lib")

using io_control_block = OVERLAPPED;

#elif __unix__ || __linux__
#include <fcntl.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

struct io_control_block
{
    int64_t sd;
    socklen_t addrlen;
};
#endif

using coroutine_task_t = std::experimental::coroutine_handle<void>;
using buffer_view_t = gsl::span<gsl::byte>;

struct _INTERFACE_ io_work_t : public io_control_block
{
    coroutine_task_t task{};
    buffer_view_t buffer{};
    union {
        sockaddr* ep{}; // endpoint
        sockaddr_in6* from6;
        const sockaddr_in6* to6;
        sockaddr_in* from;
        const sockaddr_in* to;
    };

  public:
    bool ready() const noexcept;
};
static_assert(sizeof(buffer_view_t) <= sizeof(void*) * 2);
static_assert(sizeof(io_work_t) <= 64);

class io_send_to final : public io_work_t
{
  public:
    _INTERFACE_ void suspend(coroutine_task_t rh) noexcept(false);
    _INTERFACE_ int64_t resume() noexcept;

  public:
    auto await_ready() const noexcept
    {
        return this->ready();
    }
    auto await_suspend(coroutine_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    auto await_resume() noexcept
    {
        return this->resume();
    }
};

class io_recv_from final : public io_work_t
{
  public:
    _INTERFACE_ void suspend(coroutine_task_t rh) noexcept(false);
    _INTERFACE_ int64_t resume() noexcept;

  public:
    auto await_ready() const noexcept
    {
        return this->ready();
    }
    auto await_suspend(coroutine_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    auto await_resume() noexcept
    {
        return this->resume();
    }
};

static_assert(sizeof(io_send_to) == sizeof(io_work_t));
static_assert(sizeof(io_recv_from) == sizeof(io_work_t));

_INTERFACE_
auto send_to(uint64_t sd, const sockaddr_in& remote, //
             buffer_view_t buffer, io_work_t& work) noexcept(false)
    -> io_send_to&;

_INTERFACE_
auto send_to(uint64_t sd, const sockaddr_in6& remote, //
             buffer_view_t buffer, io_work_t& work) noexcept(false)
    -> io_send_to&;

_INTERFACE_
auto recv_from(uint64_t sd, sockaddr_in6& remote, //
               buffer_view_t buffer, io_work_t& work) noexcept(false)
    -> io_recv_from&;

_INTERFACE_
auto recv_from(uint64_t sd, sockaddr_in& remote, //
               buffer_view_t buffer, io_work_t& work) noexcept(false)
    -> io_recv_from&;

#ifndef _MSC_VER

// - Note
//      Caller must continue loop without break
//      so that there is no leak of event
_INTERFACE_
auto fetch_io_tasks() noexcept(false) -> enumerable<coroutine_task_t>;

#endif

_INTERFACE_
auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>;

_INTERFACE_
auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept
    -> enumerable<sockaddr_in6>;

_INTERFACE_
uint32_t nameof(const sockaddr_in6& ep, //
                gsl::zstring<NI_MAXHOST> name) noexcept;

_INTERFACE_
uint32_t nameof(const sockaddr_in6& ep, //
                gsl::zstring<NI_MAXHOST> name,
                gsl::zstring<NI_MAXSERV> serv) noexcept;

#endif // COROUTINE_NET_IO_H