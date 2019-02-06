//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>
#include <coroutine/return.h>
#include <coroutine/sync.h>
#include <gsl/gsl>

#include "./socket.h"

using namespace std;
using namespace gsl;
using namespace std::chrono_literals;

auto coro_recv_dgram(int64_t sd, sockaddr_in6& remote, int64_t& rsz,
                     wait_group& wg) -> unplug;
auto coro_send_dgram(int64_t sd, const sockaddr_in6& remote, int64_t& ssz,
                     wait_group& wg) -> unplug;

auto echo_incoming_datagram(int64_t sd) -> unplug;

TEST_CASE("socket udp echo test", "[network][socket]")
{
    constexpr auto test_service_port = 32771;
    addrinfo hint{}; // address hint
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;

    // service socket
    int64_t ss = socket_create(hint);
    auto d1 = gsl::finally([ss]() { socket_close(ss); });

    socket_set_option_reuse_address(ss);
    socket_set_option_nonblock(ss);

    endpoint_t ep{};
    ep.in6.sin6_family = hint.ai_family;
    ep.in6.sin6_addr = in6addr_any;
    ep.in6.sin6_port = htons(test_service_port);
    socket_bind(ss, ep.in6);

    // start service
    echo_incoming_datagram(ss);

    SECTION("multiple clients")
    {
        constexpr auto max_clients = 4;

        array<int64_t, max_clients> clients{};
        array<int64_t, max_clients> recv_lengths{};
        array<int64_t, max_clients> send_lengths{};
        array<sockaddr_in6, max_clients> recv_endpoints{};

        gsl::index i = 0u;
        for (auto sd : socket_create(hint, clients.size()))
        {
            clients[i++] = sd;
            socket_set_option_nonblock(sd);
        }
        auto d2 = gsl::finally([&clients]() {
            for (auto sd : clients)
                socket_close(sd);
        });

        wait_group wg{};         // wait group for coroutine sync
        wg.add(max_clients * 2); // each client will perform
                                 // 1 recv and 1 send
        {
            // recv packets
            // later echo response will be delivered to these coroutines
            for (i = 0; i < max_clients; ++i)
                coro_recv_dgram(clients[i], recv_endpoints[i], recv_lengths[i],
                                wg);

            // send packets to service address
            ep.in6.sin6_addr = in6addr_loopback;
            for (i = 0; i < max_clients; ++i)
                coro_send_dgram(clients[i], ep.in6, send_lengths[i], wg);

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
            // !!!!!
            // unlike windows api, we have to resume tasks manually
            // the library doesn't guarantee they will be fetched at once
            // so user have to repeat enough to finish all i/o tasks
            // !!!!!
            auto count = 30;
            while (count--)
                for (auto task : wait_io_tasks(10ms))
                    task.resume();
#endif
        }
        wg.wait(4s); // ensure all coroutines are finished

        // now, receive coroutines must hold same data
        // sent by each client sockets
        for (i = 0; i < max_clients; ++i)
        {
            REQUIRE(send_lengths[i] == recv_lengths[i]);
            bool equal = memcmp(addressof(ep.in6.sin6_addr),
                                addressof(recv_endpoints[i].sin6_addr),
                                sizeof(in6_addr))
                         == 0;
            REQUIRE(equal);
        }
    }
    // test end
}

auto coro_recv_dgram(int64_t sd, sockaddr_in6& remote, int64_t& rsz,
                     wait_group& wg) -> unplug
{
    using gsl::byte;
    auto d = finally([&wg]() { // ensure noti to wait_group
        wg.done();
    });

    io_work_t work{};
    array<byte, 1253> storage{};

    rsz = co_await recv_from(sd, remote, storage, work);
    REQUIRE(rsz > 0);
    REQUIRE(work.error() == 0);
}

auto coro_send_dgram(int64_t sd, const sockaddr_in6& remote, int64_t& ssz,
                     wait_group& wg) -> unplug
{
    using gsl::byte;
    auto d = finally([&wg]() { // ensure noti to wait_group
        wg.done();
    });

    io_work_t work{};
    array<byte, 782> storage{};

    ssz = co_await send_to(sd, remote, storage, work);
    REQUIRE(ssz == storage.size());
    REQUIRE(work.error() == 0);
}

auto echo_incoming_datagram(int64_t sd) -> unplug
{
    using gsl::byte;
    io_work_t work{};
    buffer_view_t buf{};
    int64_t rsz = 0, ssz = 0;
    sockaddr_in6 remote{};
    array<byte, 3927> storage{};

    while (true)
    {
        rsz = co_await recv_from(sd, remote, buf = storage, work);
        if (work.error())
            goto OnError;

        ssz = co_await send_to(sd, remote, buf = {storage.data(), rsz}, work);
        if (work.error())
            goto OnError;
    }
    co_return;
OnError:
    auto em = std::system_category().message(work.error());
    FAIL(em);
}
