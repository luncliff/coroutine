//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>
#include <coroutine/return.h>
#include <coroutine/sync.h>
#include <gsl/gsl>

using namespace std;
using namespace gsl;
using namespace std::chrono_literals;

void create_bound_socket(int64_t& sd, sockaddr_in6& ep, const addrinfo& hint,
                         gsl::czstring<> host, gsl::czstring<> port);
auto create_bound_sockets(const addrinfo& hint, uint16_t port, size_t count)
    -> enumerable<int64_t>;
void dispose_socket(int64_t sd);
void apply_nonblock_async(int64_t sd);

auto coro_recv_dgram(int64_t sd, sockaddr_in6& remote, int64_t& rsz,
                     wait_group& wg) -> unplug;
auto coro_send_dgram(int64_t sd, const sockaddr_in6& remote, int64_t& ssz,
                     wait_group& wg) -> unplug;

auto echo_incoming_datagram(int64_t sd) -> unplug;

TEST_CASE("socket udp echo test", "[network][socket][udp]")
{
    addrinfo hint{}; // address hint
    int64_t ss = -1; // service socket
    endpoint_t ep{}; // service endpoint

    // create service socket
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICHOST | AI_NUMERICSERV;

    create_bound_socket(ss, ep.in6, hint, "::1", "32771");
    REQUIRE(ep.in6.sin6_port == htons(32771));

    // start service
    echo_incoming_datagram(ss);

    constexpr auto max_clients = 4;

    // wait group for coroutine sync
    wait_group wg{};
    // create some client sockets and data
    array<int64_t, max_clients> clients{};
    array<int64_t, max_clients> recv_lengths{};
    array<int64_t, max_clients> send_lengths{};
    array<sockaddr_in6, max_clients> recv_endpoints{};

    gsl::index i = 0u;
    for (auto sd : create_bound_sockets(hint, 64211, clients.size()))
    {
        clients[i++] = sd;
        apply_nonblock_async(sd);
    }

    // each client will perform 1 recv and 1 send
    wg.add(max_clients * 2);

    // recv packets. later echo response will resume the coroutines
    for (i = 0; i < max_clients; ++i)
        coro_recv_dgram(clients[i], recv_endpoints[i], recv_lengths[i], wg);

    // send packets to service address
    ep.in6.sin6_addr = in6addr_loopback;
    for (i = 0; i < max_clients; ++i)
        coro_send_dgram(clients[i], ep.in6, send_lengths[i], wg);

    // !!!!! unlike windows api, we have to resume tasks manually !!!!!
    // the library doesn't guarantee they will be fetched at once
    // so user have to repeat enough
    {
        auto count = 30;
        while (count--)
            for (auto task : wait_io_tasks(0ms))
                task.resume();
    }

    wg.wait(10s); // wait for coroutines

    // now, receive coroutines must hold same data
    // sent by each client sockets
    for (i = 0; i < max_clients; ++i)
    {
        REQUIRE(send_lengths[i] == recv_lengths[i]);
        bool equal
            = memcmp(addressof(ep.in6.sin6_addr),
                     addressof(recv_endpoints[i].sin6_addr), sizeof(in6_addr))
              == 0;
        REQUIRE(equal);
    }

    // close
    for (auto sd : clients)
        dispose_socket(sd);

    // stop service
    dispose_socket(ss);
}

auto coro_recv_dgram( //
    int64_t sd, sockaddr_in6& remote, int64_t& rsz, wait_group& wg) -> unplug
{
    using gsl::byte;

    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    array<byte, 1253> storage{};

    rsz = co_await recv_from(sd, remote, storage, work);
    REQUIRE(rsz > 0);
    REQUIRE(work.error() == 0);
}

auto coro_send_dgram( //
    int64_t sd, const sockaddr_in6& remote, int64_t& ssz, wait_group& wg)
    -> unplug
{
    using gsl::byte;

    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

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
