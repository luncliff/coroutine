//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//

#include <catch2/catch.hpp>

#include <coroutine/frame.h>
#include <coroutine/return.h>

#include <csignal>

#include <gsl/gsl>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// constexpr auto reserved_signal = SIGUSR2;

using packet_chunk_t = std::array<gsl::byte, 4096>;
using signal_action_t = struct sigaction;
using signal_callback_t = void (*)(int sig, siginfo_t* si, void* ctx);
void on_io_complete(int sig, siginfo_t* si, void* ctx) noexcept(false);
auto chain_previous_action() -> gsl::not_null<signal_action_t*>;

using coroutine_task_t = std::experimental::coroutine_handle<void>;
using endpoint_t = struct sockaddr_in6;
using buffer_view_t = gsl::span<gsl::byte>;

struct io_work_t
{
    void* tag = nullptr;
    int socket = -1;
    buffer_view_t buffer{};
    union {
        endpoint_t* from{};
        const endpoint_t* to;
    };
    socklen_t length = 0;

  public:
    bool ready() const noexcept;
    uint32_t resume() noexcept;
};
// uint32_t error(const io_work_t& work) noexcept;
static_assert(sizeof(io_work_t) <= 64);

class io_send_to final : public io_work_t
{
  public:
    void suspend(coroutine_task_t rh) noexcept(false);

  public:
    bool await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(coroutine_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    uint32_t await_resume() noexcept
    {
        return this->resume();
    }
};

class io_recv_from final : public io_work_t
{
  public:
    void suspend(coroutine_task_t rh) noexcept(false);

  public:
    constexpr bool await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(coroutine_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    uint32_t await_resume() noexcept
    {
        return this->resume();
    }
};

static_assert(sizeof(io_send_to) == sizeof(io_work_t));
static_assert(sizeof(io_recv_from) == sizeof(io_work_t));

auto send_to(int sd, const endpoint_t& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&;
auto recv_from(int sd, endpoint_t& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&;

TEST_CASE("async socket io", "[network]")
{
    signal_action_t io_action{};

    // restart the system calls
    // handler receives additional signal information
    io_action.sa_flags = SA_RESTART | SA_SIGINFO;
    io_action.sa_sigaction = on_io_complete;

    REQUIRE(             //
        sigaction(SIGIO, //
                  &io_action, chain_previous_action())
        == 0);
    // recover signal handler
    auto d0 = gsl::finally([=]() noexcept {
        sigaction(SIGIO, //
                  chain_previous_action(), nullptr);
    });

    CAPTURE(chain_previous_action()->sa_sigaction);

    constexpr auto max_working_io = 20;

    auto reserved_works = std::make_unique<io_work_t[]>(max_working_io);
    auto reserved_chunks = std::make_unique<packet_chunk_t[]>(max_working_io);

    auto& work = reserved_works[0];
    auto& chunk = reserved_chunks[0];

    auto sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    REQUIRE(sd != -1);
    auto d1 = gsl::finally([=]() noexcept { close(sd); });

    // Be sure to receive SIGIO
    REQUIRE(fcntl(sd, F_SETOWN, getpid()) == 0);
    // make async socket
    REQUIRE(fcntl(sd, F_SETFL, O_ASYNC | O_NONBLOCK) == 0);

    SECTION("send_to")
    {
        auto test_send_to = [](int sd, buffer_view_t buffer, endpoint_t& remote,
                               io_work_t& w) -> unplug {
            auto& w2 = send_to(sd, remote, buffer, w);

            REQUIRE(w2.tag == nullptr);

            auto res = co_await w2;
            REQUIRE(res > 0);
        };

        endpoint_t remote{};
        remote.sin6_family = AF_INET6;
        remote.sin6_addr = in6addr_loopback;
        remote.sin6_port = htons(7);

        const auto message = "this is test message";
        auto buffer = buffer_view_t{
            const_cast<gsl::byte*>(reinterpret_cast<const gsl::byte*>(message)),
            static_cast<std::ptrdiff_t>(strnlen(message, 2000))};

        test_send_to(sd, buffer, remote, work);
    }

    // SECTION("recv_from")
    // {
    //     void* status = nullptr;
    //     auto test_recv_from
    //         = [&status](int sd, buffer_view_t buffer, endpoint_t& remote,
    //                     io_work_t& w) -> unplug {
    //         auto& w2 = recv_from(sd, remote, buffer, w);
    //         REQUIRE(w2.tag == nullptr);
    //         auto res = co_await w2;
    //         REQUIRE(res > 0);
    //         status = w2.tag;
    //     };
    //     endpoint_t remote{};
    //     remote.sin6_family = AF_INET6;
    //     remote.sin6_addr = in6addr_loopback;
    //     remote.sin6_port = htons(7);
    //     auto message_buffer = std::make_unique<gsl::byte[]>(2000);
    //     auto buffer = buffer_view_t{message_buffer.get(), 2000};

    //     test_recv_from(sd, buffer, remote, work);
    //     REQUIRE(status == nullptr);
    // }
}

signal_action_t stacked_action{};

auto chain_previous_action() -> gsl::not_null<signal_action_t*>
{
    return std::addressof(stacked_action);
}
void on_io_complete(int sig, siginfo_t* si, void* ctx) noexcept(false)
{
    // invoke chained handler first
    if (auto handler = stacked_action.sa_sigaction)
        handler(sig, si, ctx);

    // if (auto task = coroutine_task_t::from_address(ctx))
    //     task.resume();
}
bool io_work_t::ready() const noexcept
{
    return false;
}

uint32_t io_work_t::resume() noexcept
{
    return this->length;
    // return static_cast<uint32_t>(aio_return(this));
}
// uint32_t error(const io_work_t& work) noexcept
// {
//     return aio_error(std::addressof(work));
// }

auto send_to(int sd, const endpoint_t& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&
{
    // if linux, flag also requires SOCK_NONBLOCK

    auto ec = fcntl(sd, F_SETFL, O_ASYNC);
    if (ec)
        throw std::system_error{ec, std::system_category(), "fcntl"};

    work.socket = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;
    return *reinterpret_cast<io_send_to*>(std::addressof(work));
}
void io_send_to::suspend(coroutine_task_t rh) noexcept(false)
{
    this->tag = rh.address();

    auto length
        = sendto(socket, buffer.data(), buffer.size_bytes(), 0,
                 reinterpret_cast<const sockaddr*>(to), sizeof(endpoint_t));

    if (length == static_cast<decltype(length)>(-1))
        throw std::system_error{errno, std::system_category(), "sendto"};
}

auto recv_from(int sd, endpoint_t& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&
{
    auto ec = fcntl(sd, F_SETFL, O_ASYNC);
    if (ec)
        throw std::system_error{ec, std::system_category(), "fcntl"};

    work.socket = sd;
    work.to = std::addressof(remote);
    work.buffer = buffer;

    return *reinterpret_cast<io_recv_from*>(std::addressof(work));
}
void io_recv_from::suspend(coroutine_task_t rh) noexcept(false)
{
    this->tag = rh.address();

    auto length = recvfrom(socket, buffer.data(), buffer.size_bytes(), 0,
                           reinterpret_cast<sockaddr*>(from),
                           std::addressof(this->length));

    if (length == static_cast<decltype(length)>(-1))
        throw std::system_error{errno, std::system_category(), "recvfrom"};
}
