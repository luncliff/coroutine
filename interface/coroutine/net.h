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
#include <coroutine/yield.hpp>

#include <chrono>
#include <gsl/gsl>

#if defined(_MSC_VER) // use winsock
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <ws2def.h>
#pragma comment(lib, "Ws2_32.lib")

using io_control_block = OVERLAPPED;

static constexpr bool is_winsock = true;
static constexpr bool is_netinet = false;

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
// use netinet
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

// follow the definition of Windows `OVERLAPPED`
struct io_control_block {
    uint64_t internal; // uint32_t errc; int32_t flag;
    uint64_t internal_high;
    union {
        struct {
            int32_t offset; // socklen_t addrlen;
            int32_t offset_high;
        };
        void* ptr;
    };
    int64_t handle; // int64_t sd;
};

static constexpr bool is_winsock = false;
static constexpr bool is_netinet = true;

#endif // winsock || netinet

//  Helper type for socket address. it will be used for simplicity
union endpoint_t final {
    sockaddr_storage storage{};
    sockaddr addr;
    sockaddr_in in4;
    sockaddr_in6 in6;
};

//  1 task item == 1 resumable function
using io_task_t = std::experimental::coroutine_handle<void>;

//  This is simply a view to storage. Be aware that it doesn't have ownership
using io_buffer_t = gsl::span<std::byte>;
static_assert(sizeof(io_buffer_t) <= sizeof(void*) * 2);

//  In short, this is a struct to describe "1 I/O request" to system API
//  All members in `io_work_t` cooperate in harmony.
//  So don't modify directly !
class io_work_t : public io_control_block {
    // todo: find a way to follow C++ Core Guideline rules
  public:
    io_task_t task{};
    io_buffer_t buffer{};
    endpoint_t* ep{};

  public:
    _INTERFACE_ bool ready() const noexcept;
    _INTERFACE_ uint32_t error() const noexcept;
};
static_assert(sizeof(io_work_t) <= 64);

//  Type to perform `sendto` I/O request
class io_send_to final : public io_work_t {
  private:
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
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
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
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
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
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
    _INTERFACE_ void suspend(io_task_t t) noexcept(false);
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

// clang-format off

//  Constructs `io_send_to` type with given parameters
[[nodiscard]] _INTERFACE_
auto send_to(uint64_t sd, const sockaddr_in& remote,           //
             io_buffer_t buf, io_work_t& work) noexcept(false) //
    -> io_send_to&;

//  Constructs `io_send_to` type with given parameters
[[nodiscard]] _INTERFACE_
auto send_to(uint64_t sd, const sockaddr_in6& remote,          //
             io_buffer_t buf, io_work_t& work) noexcept(false) //
    -> io_send_to&;

//  Constructs `io_recv_from` type with given parameters
[[nodiscard]] _INTERFACE_
auto recv_from(uint64_t sd, sockaddr_in6& remote, io_buffer_t buf, //
               io_work_t& work) noexcept(false)                    //
    -> io_recv_from&;

//  Constructs `io_recv_from` type with given parameters
[[nodiscard]] _INTERFACE_
auto recv_from(uint64_t sd, sockaddr_in& remote, io_buffer_t buf, //
               io_work_t& work) noexcept(false)                   //
    -> io_recv_from&;

//  Constructs `io_send` type with given parameters
[[nodiscard]] _INTERFACE_
auto send_stream(uint64_t sd, io_buffer_t buf, uint32_t flag, //
                 io_work_t& work) noexcept(false)             //
    -> io_send&;

//  Constructs `io_recv` type with given parameters
[[nodiscard]] _INTERFACE_
auto recv_stream(uint64_t sd, io_buffer_t buf, uint32_t flag, //
                 io_work_t& work) noexcept(false)             //
    -> io_recv&;

// clang-format on

//  This function is for non-windows platform. Over windows api, it always
//  yields nothing. User must continue loop *without break*
//  so that there is leak of event.
//
//  Also, the library doesn't guarantee all coroutines(i/o tasks) will be
//  fetched at once. Therefore it is strongly recommended for user to have
//  another method to detect that watching I/O coroutines are returned.
_INTERFACE_
auto wait_net_tasks(std::chrono::nanoseconds timeout) noexcept(false)
    -> coro::enumerable<io_task_t>;

//
//  Name resolution utilities
//

using zstring_host = gsl::zstring<NI_MAXHOST>;
using zstring_serv = gsl::zstring<NI_MAXSERV>;
using czstring_host = gsl::czstring<NI_MAXHOST>;
using czstring_serv = gsl::czstring<NI_MAXSERV>;

//  Combination of `getaddrinfo` functions
//  If there is an error, the enumerable yields nothing.
_INTERFACE_
auto resolve(const addrinfo& hint, //
             czstring_host name, czstring_serv serv) noexcept(false)
    -> coro::enumerable<endpoint_t>;

// clang-format off

//  Thin wrapper of `getnameinfo`
//  parameter `serv` can be `nullptr`.
[[nodiscard]] _INTERFACE_
int get_name(const endpoint_t& ep, //
             zstring_host name, zstring_serv serv, 
             int flags = NI_NUMERICHOST | NI_NUMERICSERV) noexcept;

// clang-format on

#endif // COROUTINE_NET_IO_H