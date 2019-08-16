//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Async I/O operation support for socket
//
#pragma once
// clang-format off
#if defined(FORCE_STATIC_LINK)
#   define _INTERFACE_
#   define _HIDDEN_
#elif defined(_MSC_VER) // MSVC or clang-cl
#   define _HIDDEN_
#   ifdef _WINDLL
#       define _INTERFACE_ __declspec(dllexport)
#   else
#       define _INTERFACE_ __declspec(dllimport)
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define _INTERFACE_ __attribute__((visibility("default")))
#   define _HIDDEN_ __attribute__((visibility("hidden")))
#else
#   error "unexpected linking configuration"
#endif
// clang-format on

#ifndef COROUTINE_NET_IO_H
#define COROUTINE_NET_IO_H

#include <chrono>
#include <gsl/gsl>
#include <coroutine/yield.hpp>

#if __has_include(<WinSock2.h>) // use winsock
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <ws2def.h>

using io_control_block = OVERLAPPED;

static constexpr bool is_winsock = true;
static constexpr bool is_netinet = false;

#elif __has_include(<netinet/in.h>) // use netinet
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

// Follow the definition of Windows `OVERLAPPED`
struct io_control_block {
    uint64_t internal;      // uint32_t errc, int32_t flag
    uint64_t internal_high; // int64_t len, socklen_t addrlen
    union {
        struct {
            int32_t offset;
            int32_t offset_high;
        };
        void* ptr; // sockaddr* addr;
    };
    int64_t handle; // int64_t sd;
};

static constexpr bool is_winsock = false;
static constexpr bool is_netinet = true;

#endif // winsock || netinet

namespace coro {
using namespace std;
using namespace std::experimental;

//  1 I/O task == 1 coroutine function
using io_task_t = coroutine_handle<void>;

//  This is simply a view to storage. Be aware that it doesn't have ownership
using io_buffer_t = gsl::span<std::byte>;
static_assert(sizeof(io_buffer_t) <= sizeof(void*) * 2);

//  A struct to describe "1 I/O request" to system API
class io_work_t : public io_control_block {
  public:
    io_task_t task{};
    io_buffer_t buffer{};

  protected:
    _INTERFACE_ bool ready() const noexcept;

  public:
    // Multiple retrieving won't be a matter
    _INTERFACE_ uint32_t error() const noexcept;
};
static_assert(sizeof(io_work_t) <= 56);

//  Type to perform `sendto` I/O request
class io_send_to final : public io_work_t {
  private:
    // This function must be used through `co_await`
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
    // This function must be used through `co_await`
    // Unlike inherited `error` function, multiple invoke of this will
    // lead to malfunction.
    _INTERFACE_ int64_t resume() noexcept;

  public:
    bool await_ready() const noexcept {
        return this->ready();
    }
    void await_suspend(io_task_t t) noexcept(false) {
        return this->suspend(t);
    }
    auto await_resume() noexcept {
        return this->resume();
    }
};
static_assert(sizeof(io_send_to) == sizeof(io_work_t));

//  Type to perform `recvfrom` I/O request
class io_recv_from final : public io_work_t {
  private:
    // This function must be used through `co_await`
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
    // This function must be used through `co_await`
    // Unlike inherited `error` function, multiple invoke of this will
    // lead to malfunction.
    _INTERFACE_ int64_t resume() noexcept;

  public:
    bool await_ready() const noexcept {
        return this->ready();
    }
    void await_suspend(io_task_t t) noexcept(false) {
        return this->suspend(t);
    }
    auto await_resume() noexcept {
        return this->resume();
    }
};
static_assert(sizeof(io_recv_from) == sizeof(io_work_t));

//  Type to perform `send` I/O request
class io_send final : public io_work_t {
  private:
    // This function must be used through `co_await`
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
    // This function must be used through `co_await`
    // Unlike inherited `error` function, multiple invoke of this will
    // lead to malfunction.
    _INTERFACE_ int64_t resume() noexcept;

  public:
    bool await_ready() const noexcept {
        return this->ready();
    }
    void await_suspend(io_task_t t) noexcept(false) {
        return this->suspend(t);
    }
    auto await_resume() noexcept {
        return this->resume();
    }
};
static_assert(sizeof(io_send) == sizeof(io_work_t));

//  Type to perform `recv` I/O request
class io_recv final : public io_work_t {
  private:
    // This function must be used through `co_await`
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
    // This function must be used through `co_await`
    // Unlike inherited `error` function, multiple invoke of this will
    // lead to malfunction.
    _INTERFACE_ int64_t resume() noexcept;

  public:
    bool await_ready() const noexcept {
        return this->ready();
    }
    void await_suspend(io_task_t t) noexcept(false) {
        return this->suspend(t);
    }
    auto await_resume() noexcept {
        return this->resume();
    }
};
static_assert(sizeof(io_recv) == sizeof(io_work_t));

//  Constructs awaitable `io_send_to` object with the given parameters
[[nodiscard]] _INTERFACE_                                     //
    auto                                                      //
    send_to(uint64_t sd, const sockaddr_in& remote,           //
            io_buffer_t buf, io_work_t& work) noexcept(false) //
    -> io_send_to&;

//  Constructs awaitable `io_send_to` object with the given parameters
[[nodiscard]] _INTERFACE_                                             //
    auto                                                              //
    send_to(uint64_t sd, const sockaddr_in6& remote, io_buffer_t buf, //
            io_work_t& work) noexcept(false)                          //
    -> io_send_to&;

//  Constructs awaitable `io_recv_from` object with the given parameters
[[nodiscard]] _INTERFACE_                                         //
    auto                                                          //
    recv_from(uint64_t sd, sockaddr_in6& remote, io_buffer_t buf, //
              io_work_t& work) noexcept(false)                    //
    -> io_recv_from&;

//  Constructs awaitable `io_recv_from` object with the given parameters
[[nodiscard]] _INTERFACE_                                        //
    auto                                                         //
    recv_from(uint64_t sd, sockaddr_in& remote, io_buffer_t buf, //
              io_work_t& work) noexcept(false)                   //
    -> io_recv_from&;

//  Constructs awaitable `io_send` object with the given parameters
[[nodiscard]] _INTERFACE_                                    //
    auto                                                     //
    send_stream(uint64_t sd, io_buffer_t buf, uint32_t flag, //
                io_work_t& work) noexcept(false)             //
    -> io_send&;

//  Constructs awaitable `io_recv` object with the given parameters
[[nodiscard]] _INTERFACE_                                    //
    auto                                                     //
    recv_stream(uint64_t sd, io_buffer_t buf, uint32_t flag, //
                io_work_t& work) noexcept(false)             //
    -> io_recv&;

//  This function is for non-Windows platform.
//  Over Windows api, it always yields **nothing**.
//
//  Its caller must continue the loop without break
//  so there is no leak of the I/O events
//
//  Also, the library doesn't guarantee all coroutines(i/o tasks) will be
//  fetched at once. Therefore it is strongly recommended for user to have
//  another method to detect that watching I/O coroutines are returned.
_INTERFACE_
void wait_net_tasks(enumerable<io_task_t>& tasks,
                    chrono::nanoseconds timeout) noexcept(false);

inline auto wait_net_tasks(chrono::nanoseconds timeout) noexcept(false) {
    enumerable<io_task_t> tasks{};
    wait_net_tasks(tasks, timeout);
    return tasks;
}

//
//  Name resolution utilities
//

using zstring_host = gsl::zstring<NI_MAXHOST>;
using zstring_serv = gsl::zstring<NI_MAXSERV>;
using czstring_host = gsl::czstring<NI_MAXHOST>;
using czstring_serv = gsl::czstring<NI_MAXSERV>;

//  Combination of `getaddrinfo` functions
//  If there is an error, the enumerable is untouched
_INTERFACE_
int32_t resolve(enumerable<sockaddr>& g, const addrinfo& hint, //
                czstring_host name, czstring_serv serv) noexcept;

// construct system_error using `gai_strerror` function
_INTERFACE_ auto resolve_error(int32_t ec) noexcept -> std::system_error;

inline auto resolve(const addrinfo& hint, //
                    czstring_host name, czstring_serv serv) noexcept(false)
    -> enumerable<sockaddr> {
    enumerable<sockaddr> g{};
    if (const auto ec = resolve(g, hint, name, serv)) {
        throw resolve_error(ec);
    }
    return g;
}

//  Thin wrapper of `getnameinfo`. Parameter 'serv' can be nullptr.
_INTERFACE_
int32_t get_name(const sockaddr_in& addr, zstring_host name, zstring_serv serv,
                 int32_t flags = NI_NUMERICHOST | NI_NUMERICSERV) noexcept;
_INTERFACE_
int32_t get_name(const sockaddr_in6& addr, zstring_host name, zstring_serv serv,
                 int32_t flags = NI_NUMERICHOST | NI_NUMERICSERV) noexcept;

} // namespace coro

#endif // COROUTINE_NET_IO_H
