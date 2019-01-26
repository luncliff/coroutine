//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//

#include <catch2/catch.hpp>

#include <coroutine/frame.h>
#include <coroutine/return.h>

#include "net/async.h"

#include <future>

TEST_CASE("async network api", "[network]")
{
    auto apply_nonblock_async = [](auto sd) {
        REQUIRE(fcntl(sd, F_SETFL, // non block
                                   //  allow sigio redirection
                      O_NONBLOCK | O_ASYNC | fcntl(sd, F_GETFL, 0))
                != -1);
    };

    auto rs = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    REQUIRE(rs > 0);
    auto d1 = gsl::finally([=]() noexcept { close(rs); });
    apply_nonblock_async(rs);

    auto ws = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    REQUIRE(ws > 0);
    auto d2 = gsl::finally([=]() noexcept { close(ws); });
    apply_nonblock_async(ws);

    auto coro_recv_from = [](int sd,              //
                             sockaddr_in& remote, //
                             packet_chunk_t& buffer) -> unplug {
        io_work_t wk{};
        auto rsz = co_await recv_from(sd, remote, buffer, wk);
        printf("recv_from %ld \n", rsz);
    };
    auto coro_send_to = [](int sd,                    //
                           const sockaddr_in& remote, //
                           packet_chunk_t& buffer) -> unplug {
        io_work_t wk{};
        auto ssz = co_await send_to(sd, remote, buffer, wk);
        printf("sendto %ld \n", ssz);
    };

    SECTION("coro send recv")
    {
        auto chunk = std::make_unique<packet_chunk_t>();

        // service address and bind
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port = htons(11320);
        local.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(rs, (sockaddr*)&local, sizeof(sockaddr_in)) == -1)
            FAIL(strerror(errno));

        sockaddr_in remote{};
        remote.sin_family = AF_INET;
        remote.sin_port = htons(11320);
        remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        coro_recv_from(rs, local, *chunk);
        coro_send_to(ws, remote, *chunk);

        auto count = 0;
        while (count < 2) // the library recommends the users
                          // to have a proper criteria
                          // so that there is no leak of event
            for (auto task : fetch_io_tasks())
            {
                task.resume();
                ++count;
            }
        REQUIRE(count == 2);
    }
}

TEST_CASE("temp")
{

    // SECTION("non blocking recvfrom")
    // {
    //     auto sz = recvfrom(sd, //
    //                        chunk->data(), chunk->size(), 0, nullptr,
    //                        nullptr);
    //     REQUIRE(sz == -1);
    //     REQUIRE(errno == EAGAIN);
    // }

    // SECTION("non blocking sendto")
    // {
    //     endpoint_t remote{};
    //     remote.sin6_family = AF_INET;
    //     remote.sin6_port = htons(7); // use echo port
    //     remote.sin6_addr = in6addr_loopback;
    //     auto sz = sendto(sd, //
    //                      chunk->data(), chunk->size(), 0,
    //                      reinterpret_cast<sockaddr*>(&remote),
    //                      sizeof(sockaddr_in6));
    //     if (sz == -1 && errno != EAGAIN)
    //         FAIL(strerror(errno));
    //     else
    //         REQUIRE(sz > 0);
    // }

    // SECTION("sync send recv")
    // {
    //     auto sync_recv_from = [](int sd, packet_chunk_t& chunk) -> void {
    //         sockaddr_in from{};
    //         // endpoint_t from{};
    //         socklen_t asz = sizeof(sockaddr_in);
    //         auto rsz = recvfrom(sd, chunk.data(), chunk.size(), 0,
    //                             (sockaddr*)&from, &asz);
    //         // if (rsz == -1)
    //         //     FAIL(strerror(errno));
    //         // REQUIRE(rsz > 0);
    //         printf("recvfrom %ld \n", rsz);
    //     };
    //     auto sync_send_to = [](int sd,                    //
    //                            const sockaddr_in& remote, //
    //                            packet_chunk_t& chunk) -> void {
    //         auto ssz = sendto(sd, chunk.data(), chunk.size(), 0,
    //                           (const sockaddr*)&remote, sizeof(sockaddr_in));
    //         // if (ssz == -1)
    //         //     FAIL(strerror(errno));
    //         // REQUIRE(ssz > 0);
    //         printf("sendto %ld \n", ssz);
    //     };
    //     auto rs = sd;
    //     sockaddr_in local{};
    //     local.sin_family = AF_INET;
    //     local.sin_port = htons(11320);
    //     local.sin_addr.s_addr = htonl(INADDR_ANY);

    //     if (bind(rs, (sockaddr*)&local, sizeof(sockaddr_in)) == -1)
    //         FAIL(strerror(errno));

    //     auto ws = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //     auto d2 = gsl::finally([=]() { close(ws); });
    //     apply_nonblock_async(ws);

    //     sockaddr_in remote{};
    //     remote.sin_family = AF_INET;
    //     remote.sin_port = htons(11320);
    //     remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    //     sync_recv_from(rs, *chunk);
    //     sync_send_to(ws, remote, *chunk);
    //     sync_recv_from(rs, *chunk);
    // }
}