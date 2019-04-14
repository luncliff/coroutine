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
// clang-format off
#ifdef USE_STATIC_LINK_MACRO // ignore macro declaration in static build
#   define _INTERFACE_
#   define _HIDDEN_
#else 
#   if defined(_MSC_VER) // MSVC
#       define _HIDDEN_
#       ifdef _WINDLL
#           define _INTERFACE_ __declspec(dllexport)
#       else
#           define _INTERFACE_ __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) || defined(__clang__)
#       define _INTERFACE_ __attribute__((visibility("default")))
#       define _HIDDEN_ __attribute__((visibility("hidden")))
#   else
#       error "unexpected compiler"
#   endif // compiler check
#endif
// clang-format on

#ifndef COROUTINE_NET_IO_H
#define COROUTINE_NET_IO_H
#include <coroutine/yield.hpp>

#include <chrono>
#include <gsl/gsl>

#if defined(_MSC_VER)
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <ws2def.h>
#pragma comment(lib, "Ws2_32.lib")

using io_control_block = OVERLAPPED;

static constexpr bool is_winsock = true;
static constexpr bool is_netinet = false;
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// - Note
// follow the definition of Windows `OVERLAPPED`
struct io_control_block
{
    uint64_t internal; // uint32_t errc; int32_t flag;
    uint64_t internal_high;
    union {
        struct
        {
            int32_t offset; // socklen_t addrlen;
            int32_t offset_high;
        };
        void* ptr;
    };
    int64_t handle; // int64_t sd;
};

static constexpr bool is_winsock = false;
static constexpr bool is_netinet = true;
#endif

// - Note
//      Even though this class violates C++ Core Guidelines,
//      it will be used for simplicity
union endpoint_t final {
    sockaddr_storage storage{};
    sockaddr addr;
    sockaddr_in in4;
    sockaddr_in6 in6;
};

using io_task_t = std::experimental::coroutine_handle<void>;
using buffer_view_t = gsl::span<gsl::byte>;

struct io_work_t : public io_control_block
{
    io_task_t task{};
    buffer_view_t buffer{};
    endpoint_t* ep{};

  public:
    _INTERFACE_ bool ready() const noexcept;
    _INTERFACE_ uint32_t error() const noexcept;
};
static_assert(sizeof(buffer_view_t) <= sizeof(void*) * 2);
static_assert(sizeof(io_work_t) <= 64);

class io_send_to final : public io_work_t
{
  public:
    _INTERFACE_ void suspend(io_task_t rh) noexcept(false);
    _INTERFACE_ int64_t resume() noexcept;

  public:
    auto await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(io_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    auto await_resume() noexcept
    {
        return this->resume();
    }
};
static_assert(sizeof(io_send_to) == sizeof(io_work_t));

[[nodiscard]] _INTERFACE_ auto
    send_to(uint64_t sd, const sockaddr_in& remote, //
            buffer_view_t buffer, io_work_t& work) noexcept(false)
        -> io_send_to&;

[[nodiscard]] _INTERFACE_ //
    auto
    send_to(uint64_t sd, const sockaddr_in6& remote, //
            buffer_view_t buffer, io_work_t& work) noexcept(false)
        -> io_send_to&;

class io_recv_from final : public io_work_t
{
  public:
    _INTERFACE_ void suspend(io_task_t rh) noexcept(false);
    _INTERFACE_ int64_t resume() noexcept;

  public:
    auto await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(io_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    auto await_resume() noexcept
    {
        return this->resume();
    }
};
static_assert(sizeof(io_recv_from) == sizeof(io_work_t));

[[nodiscard]] _INTERFACE_ //
    auto
    recv_from(uint64_t sd, sockaddr_in6& remote, //
              buffer_view_t buffer, io_work_t& work) noexcept(false)
        -> io_recv_from&;

[[nodiscard]] _INTERFACE_ //
    auto
    recv_from(uint64_t sd, sockaddr_in& remote, //
              buffer_view_t buffer, io_work_t& work) noexcept(false)
        -> io_recv_from&;

class io_send final : public io_work_t
{
  public:
    _INTERFACE_ void suspend(io_task_t rh) noexcept(false);
    _INTERFACE_ int64_t resume() noexcept;

  public:
    auto await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(io_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    auto await_resume() noexcept
    {
        return this->resume();
    }
};
static_assert(sizeof(io_send) == sizeof(io_work_t));

[[nodiscard]] _INTERFACE_ //
    auto
    send_stream(uint64_t sd, buffer_view_t buffer, uint32_t flag,
                io_work_t& work) noexcept(false) -> io_send&;

class io_recv final : public io_work_t
{
  public:
    _INTERFACE_ void suspend(io_task_t rh) noexcept(false);
    _INTERFACE_ int64_t resume() noexcept;

  public:
    auto await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(io_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    auto await_resume() noexcept
    {
        return this->resume();
    }
};
static_assert(sizeof(io_recv) == sizeof(io_work_t));

[[nodiscard]] _INTERFACE_ //
    auto
    recv_stream(uint64_t sd, buffer_view_t buffer, uint32_t flag,
                io_work_t& work) noexcept(false) -> io_recv&;

// - Note
//      This function is only non-windows platform.
//      Over windows api, it always yields nothing.
//
//      User must continue looping without break so that there is no leak of
//      event. Also, the library doesn't guarantee all coroutines(i/o tasks)
//      will be fetched at once. Therefore it is strongly recommended for user
//      to have a method to detect that coroutines are returned.
_INTERFACE_
auto wait_io_tasks(std::chrono::nanoseconds timeout) noexcept(false)
    -> coro::enumerable<io_task_t>;

_INTERFACE_
auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>;

_INTERFACE_
std::errc peer_name(uint64_t sd, sockaddr_in6& ep) noexcept;

_INTERFACE_
std::errc sock_name(uint64_t sd, sockaddr_in6& ep) noexcept;

_INTERFACE_
auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept
    -> coro::enumerable<sockaddr_in6>;

_INTERFACE_
std::errc nameof(const sockaddr_in& ep, //
                 gsl::zstring<NI_MAXHOST> name) noexcept;

_INTERFACE_
std::errc nameof(const sockaddr_in6& ep, //
                 gsl::zstring<NI_MAXHOST> name) noexcept;

_INTERFACE_
std::errc nameof(const sockaddr_in6& ep, //
                 gsl::zstring<NI_MAXHOST> name,
                 gsl::zstring<NI_MAXSERV> serv) noexcept;

#endif // COROUTINE_NET_IO_H