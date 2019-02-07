// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <gsl/gsl>

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace gsl;

void create_bound_socket(SOCKET& sd, sockaddr_in6& ep, const addrinfo& hint,
                         gsl::czstring<> host, gsl::czstring<> port);

auto coro_recv_dgram(SOCKET sd, sockaddr_in6& remote, int64_t& rsz,
                     wait_group& wg) -> return_ignore;
auto coro_send_dgram(SOCKET sd, const sockaddr_in6& remote, int64_t& ssz,
                     wait_group& wg) -> return_ignore;

auto echo_incoming_datagram(SOCKET sd) -> return_ignore;

//  - Note
//      UDP Echo Server/Client
class socket_udp_echo_test : public TestClass<socket_udp_echo_test>
{
    addrinfo hint{};            // address hint
    SOCKET ss = INVALID_SOCKET; // service socket
    endpoint_t ep{};            // service endpoint

  public:
    void start_service();
    void stop_service();

    TEST_METHOD_INITIALIZE(setup)
    {
        // create service socket
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_protocol = IPPROTO_UDP;
        hint.ai_flags
            = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICHOST | AI_NUMERICSERV;

        create_bound_socket(ss, ep.in6, hint, "::1", "32771");
        Assert::IsTrue(ep.in6.sin6_port == htons(32771));

        start_service();
    }

    TEST_METHOD_CLEANUP(teardown)
    {
        stop_service();
    }

    auto create_async_sockets(size_t count) -> enumerable<SOCKET>;

    TEST_METHOD(socket_udp_echo)
    {
        constexpr auto max_clients = 4;

        // wait group for coroutine sync
        wait_group wg{};
        // create some client sockets and data
        array<SOCKET, max_clients> clients{};
        array<int64_t, max_clients> recv_lengths{};
        array<int64_t, max_clients> send_lengths{};
        array<sockaddr_in6, max_clients> recv_endpoints{};

        index i = 0u;
        for (auto sd : create_async_sockets(clients.size()))
            clients[i++] = sd;

        // send to echo service
        ep.in6.sin6_addr = in6addr_loopback;

        // each client will perform 1 recv and 1 send
        wg.add(max_clients * 2);

        // recv packets. later echo response will resume the coroutines
        for (i = 0; i < max_clients; ++i)
            coro_recv_dgram(clients[i], recv_endpoints[i], recv_lengths[i], wg);

        // send packets to service address
        for (i = 0; i < max_clients; ++i)
            coro_send_dgram(clients[i], ep.in6, send_lengths[i], wg);

        // wait for coroutines
        wg.wait(10s);

        // now, receive coroutines must hold same data
        // sent by each client sockets
        for (i = 0; i < max_clients; ++i)
        {
            Assert::IsTrue(send_lengths[i] == recv_lengths[i]);
            Assert::IsTrue(
                IN6_ADDR_EQUAL(addressof(ep.in6.sin6_addr),
                               addressof(recv_endpoints[i].sin6_addr)));
        }

        // close
        for (auto sd : clients)
        {
            shutdown(sd, SD_BOTH);
            closesocket(sd);
        }
    }
};

void socket_udp_echo_test::start_service()
{
    echo_incoming_datagram(ss);
}

void socket_udp_echo_test::stop_service()
{
    shutdown(ss, SD_RECEIVE);
    closesocket(ss);
}

auto socket_udp_echo_test::create_async_sockets(size_t count)
    -> enumerable<SOCKET>
{
    SOCKET sd = INVALID_SOCKET;
    endpoint_t ep{};

    Assert::IsTrue(hint.ai_family == AF_INET6);

    ep.in6.sin6_family = hint.ai_family;
    ep.in6.sin6_addr = in6addr_any;
    ep.in6.sin6_port = htons(64211);

    while (count--)
    {
        // use address hint in test class
        sd = ::WSASocketW(hint.ai_family, hint.ai_socktype, hint.ai_protocol,
                          nullptr, 0, WSA_FLAG_OVERLAPPED);
        Assert::IsTrue(sd != INVALID_SOCKET);

        // datagram socket need to be bound to operate
        ep.in6.sin6_port = htons(64211 + count);

        // bind socket and address
        Assert::IsTrue( //
            NO_ERROR == ::bind(sd, addressof(ep.addr), sizeof(sockaddr_in6)));

        co_yield sd;
    }
}

auto coro_recv_dgram( //
    SOCKET sd, sockaddr_in6& remote, int64_t& rsz, wait_group& wg)
    -> return_ignore
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    array<byte, 1253> storage{};

    rsz = co_await recv_from(sd, remote, storage, work);
    Assert::IsTrue(rsz > 0);
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto coro_send_dgram( //
    SOCKET sd, const sockaddr_in6& remote, int64_t& ssz, wait_group& wg)
    -> return_ignore
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    array<byte, 782> storage{};

    ssz = co_await send_to(sd, remote, storage, work);
    Assert::IsTrue(ssz == storage.size());
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto echo_incoming_datagram(SOCKET sd) -> return_ignore
{
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
    Logger::WriteMessage(em.c_str());
}
