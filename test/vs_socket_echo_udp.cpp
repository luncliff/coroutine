// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>
#include <coroutine/return.h>

#include "./socket_test.h"

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace coro;

using wait_group = concrt::latch;

auto coro_recv_dgram(SOCKET sd, sockaddr_in6& remote, int64_t& rsz,
                     wait_group& wg) -> no_return;
auto coro_send_dgram(SOCKET sd, const sockaddr_in6& remote, int64_t& ssz,
                     wait_group& wg) -> no_return;

auto echo_incoming_datagram(SOCKET sd) -> no_return;

//  - Note
//      UDP Echo Server/Client
class socket_udp_echo_test : public TestClass<socket_udp_echo_test> {
    addrinfo hint{};            // address hint
    SOCKET ss = INVALID_SOCKET; // service socket
    endpoint_t ep{};            // service endpoint

  public:
    void start_service();
    void stop_service();

    TEST_METHOD_INITIALIZE(setup) {
        // create service socket
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_protocol = IPPROTO_UDP;
        hint.ai_flags =
            AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICHOST | AI_NUMERICSERV;

        ss = static_cast<SOCKET>(socket_create(hint));

        auto af = ep.storage.ss_family = hint.ai_family;
        if (af == AF_INET) {
            ep.in4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ep.in4.sin_port = htons(32771);
        } else if (af == AF_INET6) {
            ep.in6.sin6_addr = in6addr_loopback;
            ep.in6.sin6_port = htons(32771);
        }
        socket_bind(ss, ep);

        start_service();
    }

    TEST_METHOD_CLEANUP(teardown) {
        stop_service();
    }

    TEST_METHOD(socket_udp_echo) {
        constexpr auto max_clients = 4;

        // create some client sockets and data
        array<SOCKET, max_clients> clients{};
        array<int64_t, max_clients> recv_lengths{};
        array<int64_t, max_clients> send_lengths{};
        array<sockaddr_in6, max_clients> recv_endpoints{};

        gsl::index i = 0u;
        for (auto sd : socket_create(hint, clients.size())) {
            clients[i++] = sd;

            endpoint_t local{};
            local.in6.sin6_family = AF_INET6;
            local.in6.sin6_addr = in6addr_any;
            local.in6.sin6_port = 0; // let system define the port
            socket_bind(sd, local);
        }

        // wait group for coroutine sync
        // each client will perform 1 recv and 1 send
        wait_group wg{max_clients * 2};

        // recv packets. later echo response will resume the coroutines
        for (i = 0; i < max_clients; ++i)
            coro_recv_dgram(clients[i], recv_endpoints[i], recv_lengths[i], wg);

        // send packets to service address
        ep.in6.sin6_addr = in6addr_loopback;
        for (i = 0; i < max_clients; ++i)
            coro_send_dgram(clients[i], ep.in6, send_lengths[i], wg);

        // wait for coroutines
        wg.wait();

        // now, receive coroutines must hold same data
        // sent by each client sockets
        for (i = 0; i < max_clients; ++i) {
            Assert::IsTrue(send_lengths[i] == recv_lengths[i]);
            Assert::IsTrue(
                IN6_ADDR_EQUAL(addressof(ep.in6.sin6_addr),
                               addressof(recv_endpoints[i].sin6_addr)));
        }

        // close
        for (auto sd : clients)
            socket_close(sd);
    }
};

void socket_udp_echo_test::start_service() {
    echo_incoming_datagram(ss);
}

void socket_udp_echo_test::stop_service() {
    socket_close(ss);
}

auto coro_recv_dgram(SOCKET sd, sockaddr_in6& remote, int64_t& rsz,
                     wait_group& wg) -> no_return {
    // ensure noti to wait_group
    auto d = gsl::finally([&wg]() { wg.count_down(); });

    io_work_t work{};
    array<byte, 1253> storage{};

    rsz = co_await recv_from(sd, remote, storage, work);
    Assert::IsTrue(rsz > 0);
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto coro_send_dgram(SOCKET sd, const sockaddr_in6& remote, int64_t& ssz,
                     wait_group& wg) -> no_return {
    // ensure noti to wait_group
    auto d = gsl::finally([&wg]() { wg.count_down(); });

    io_work_t work{};
    array<byte, 782> storage{};

    ssz = co_await send_to(sd, remote, storage, work);
    Assert::IsTrue(ssz == storage.size());
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto echo_incoming_datagram(SOCKET sd) -> no_return {
    io_work_t work{};
    buffer_view_t buf{};
    int64_t rsz = 0, ssz = 0;
    sockaddr_in6 remote{};
    array<byte, 3927> storage{};

    while (true) {
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
    Logger::WriteMessage(em.c_str());
}
