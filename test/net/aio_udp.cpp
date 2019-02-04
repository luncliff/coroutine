//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>
#include <coroutine/return.h>

#include <thread>

#include <fcntl.h>
#include <unistd.h>

using packet_chunk_t = std::array<gsl::byte, 2842>;
using namespace std::chrono_literals;

extern int make_async_socket(int family, int type, int proto) noexcept(false);
extern void dispose_socket(uint64_t sd);

auto coro_recv_from = [](int sd, auto& remote, //
                         packet_chunk_t& buffer) -> unplug {
    io_work_t wk{};
    int64_t rsz = co_await recv_from(sd, remote, buffer, wk);
    if (rsz < 0)
        FAIL(strerror(errno));

    REQUIRE(static_cast<size_t>(rsz) == buffer.size());
};

auto coro_send_to = [](int sd, const auto& remote, //
                       packet_chunk_t& buffer) -> unplug {
    io_work_t wk{};
    int64_t ssz = co_await send_to(sd, remote, buffer, wk);
    if (ssz < 0)
        FAIL(strerror(errno));

    REQUIRE(static_cast<size_t>(ssz) == buffer.size());
};

auto coro_recv_stream
    = [](int sd, packet_chunk_t& buffer, uint32_t flag) -> unplug {
    io_work_t wk{};
    int64_t rsz = co_await recv_stream(sd, buffer, flag, wk);
    if (rsz < 0)
        FAIL(strerror(errno));

    REQUIRE(static_cast<size_t>(rsz) <= buffer.size());
};

auto coro_send_stream
    = [](int sd, packet_chunk_t& buffer, uint32_t flag) -> unplug {
    io_work_t wk{};
    int64_t ssz = co_await send_stream(sd, buffer, flag, wk);
    if (ssz < 0)
        FAIL(strerror(errno));

    REQUIRE(static_cast<size_t>(ssz) <= buffer.size());
};

SCENARIO("async udp socket", "[network][socket]")
{
    std::thread io_worker{}; // I/O thread
    auto chunk = std::make_unique<packet_chunk_t>();
    auto addr = std::make_unique<sockaddr_storage>();

    addrinfo hint{};

    WHEN("request recv before request send")
    THEN("send is finished before recv")
    {
        int sd = -1;

        auto d1 = gsl::finally([=]() {
            if (sd != -1)
                dispose_socket(sd);
        });

        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_protocol = IPPROTO_UDP;

        sd = make_async_socket(hint.ai_family, hint.ai_socktype,
                               hint.ai_protocol);

        auto& ep = *reinterpret_cast<sockaddr_in6*>(addr.get());

        hint.ai_flags = AI_ALL | AI_NUMERICSERV | AI_NUMERICHOST;
        for (auto e : resolve(hint, "::1", "7987"))
        {
            ep = e;
            break;
        }
        // ensure non-zero
        REQUIRE(ep.sin6_port == htons(7987));

        if (bind(sd, (sockaddr*)&ep, sizeof(ep)) == -1)
            FAIL(strerror(errno));

        // Regardless of system API support, this library doesn't guarantee
        //   parallel send or recv for a socket.
        // That is, the user *can* request recv and write at some timepoint,
        //   but at most 1 work is available for each operation
        coro_recv_from(sd, ep, *chunk);
        coro_send_to(sd, ep, *chunk);

        io_worker = std::thread{[]() {
            auto count = 0;
            // the library recommends to continue loop without break
            // so that there is no leak of event
            while (count < 2)
                for (auto task : wait_io_tasks(100ms))
                    if (task)
                    {
                        task.resume();
                        ++count;
                    }

            REQUIRE(count == 2);
        }};
    }

    REQUIRE_NOTHROW(io_worker.join());
}

#if defined(_MSC_VER)

void dispose_socket(uint64_t sd)
{
    closesocket(sd);
}

void apply_nonblock_async(int64_t sd)
{
    u_long mode = TRUE;
    REQUIRE(ioctlsocket(sd, FIONBIO, &mode) == NO_ERROR);
};

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)

int make_async_socket(int family, int type, int proto) noexcept(false)
{
    int sd = socket(family, type, proto);
    if (sd == -1)
        WARN(strerror(errno));

    REQUIRE(sd > 0);
    REQUIRE(fcntl(sd, F_SETFL, // non block
                               //  allow sigio redirection
                  O_NONBLOCK | O_ASYNC | fcntl(sd, F_GETFL, 0))
            != -1);
    return sd;
}

void dispose_socket(uint64_t sd)
{
    close(sd);
}

#endif