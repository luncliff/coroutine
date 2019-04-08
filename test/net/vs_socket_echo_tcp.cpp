// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/concurrency_adapter.h>

#include "./socket_test.h"

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace std;
using namespace std::chrono_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

auto coro_recv_stream(SOCKET sd, int64_t& rsz, cc::latch& wg) -> return_ignore;
auto coro_send_stream(SOCKET sd, int64_t& ssz, cc::latch& wg) -> return_ignore;
auto echo_incoming_stream(SOCKET sd) -> return_ignore;

//  - Note
//      TCP Echo Server/Client
class socket_tcp_echo_test : public TestClass<socket_tcp_echo_test>
{
    addrinfo hint{};            // address hint
    SOCKET ln = INVALID_SOCKET; // listener socket
    endpoint_t ep{};            // service endpoint

  public:
    void start_listen();
    void stop_listen();
    void accept_dials();

    TEST_METHOD_INITIALIZE(setup)
    {
        // create listener socket
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

        ln = static_cast<SOCKET>(socket_create(hint));

        auto af = ep.storage.ss_family = hint.ai_family;
        if (af == AF_INET)
        {
            ep.in4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ep.in4.sin_port = htons(32345);
        }
        else if (af == AF_INET6)
        {
            ep.in6.sin6_addr = in6addr_loopback;
            ep.in6.sin6_port = htons(32345);
        }
        socket_bind(ln, ep.storage);
        socket_set_option_nonblock(ln);

        start_listen();
    }

    TEST_METHOD_CLEANUP(teardown)
    {
        stop_listen();
    }

    TEST_METHOD(socket_tcp_echo)
    {
        constexpr auto max_clients = 4;

        // create some client sockets and data
        array<SOCKET, max_clients> clients{};
        array<int64_t, max_clients> recv_lengths{};
        array<int64_t, max_clients> send_lengths{};

        gsl::index i = 0u;
        for (auto sd : socket_create(hint, clients.size()))
            clients[i++] = sd;

        // connect and option settings
        ep.in6.sin6_addr = in6addr_loopback;
        for (auto sd : clients)
        {
            connect(sd, addressof(ep.addr), sizeof(sockaddr_in6));
            auto ec = WSAGetLastError();
            Assert::IsTrue(ec == NO_ERROR);
            // no option settings for now ...
        }

        //
        // we can run some background accept thread with
        //  `start_listen` and `stop_listen`, but
        //  we will just accept here to prevent complex debuging
        //  sourced from parallel execution of vstest
        //
        accept_dials();

        // wait group for coroutine sync
        // each client will perform 1 recv and 1 send
        cc::latch wg{max_clients * 2};

        // recv packets. later echo response will resume the coroutines
        for (i = 0; i < max_clients; ++i)
            coro_recv_stream(clients[i], recv_lengths[i], wg);

        // send packets
        for (i = 0; i < max_clients; ++i)
            coro_send_stream(clients[i], send_lengths[i], wg);

        // wait for coroutines
        wg.wait();

        // now, receive coroutines must hold same data
        // sent by each client sockets
        for (i = 0; i < max_clients; ++i)
            Assert::IsTrue(send_lengths[i] == recv_lengths[i]);

        // close
        for (auto sd : clients)
            socket_close(sd);
    }
};

void socket_tcp_echo_test::accept_dials()
{
    SOCKET cs = INVALID_SOCKET;
    int ec = NO_ERROR;
    while (true)
    {
        // notice that `ln` is non-blocking socket.
        cs = accept(ln, nullptr, nullptr);
        ec = WSAGetLastError();

        if (ec == EWOULDBLOCK)
            continue;

        if (cs == INVALID_SOCKET) // accept failed
            // auto em = std::system_category().message(ec);
            // Logger::WriteMessage(em.c_str());
            return;

        echo_incoming_stream(cs);
    }
}

void socket_tcp_echo_test::start_listen()
{
    auto ec = WSAGetLastError();

    ec = listen(ln, 7);
    Assert::IsTrue(ec == NO_ERROR);

    // make it non-blocking
    u_long mode = true;
    Assert::IsTrue(ioctlsocket(ln, FIONBIO, &mode) == NO_ERROR);
}

void socket_tcp_echo_test::stop_listen()
{
    shutdown(ln, SD_BOTH);
    closesocket(ln);
}

auto coro_recv_stream(SOCKET sd, int64_t& rsz, cc::latch& wg) -> return_ignore
{
    // ensure noti to cc::latch
    auto d = gsl::finally([&wg]() { wg.count_down(); });

    io_work_t work{};
    array<byte, 2000> storage{};

    rsz = co_await recv_stream(sd, storage, 0, work);
    Assert::IsTrue(rsz > 0);
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto coro_send_stream(SOCKET sd, int64_t& ssz, cc::latch& wg) -> return_ignore
{
    // ensure noti to cc::latch
    auto d = gsl::finally([&wg]() { wg.count_down(); });

    io_work_t work{};
    array<byte, 1523> storage{};

    ssz = co_await send_stream(sd, storage, 0, work);
    Assert::IsTrue(ssz > 0);
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto echo_incoming_stream(SOCKET sd) -> return_ignore
{
    auto d = gsl::finally([=]() { socket_close(sd); });

    io_work_t work{};
    buffer_view_t buf{};
    int64_t rsz = 0, ssz = 0;
    array<byte, 3900> storage{};

RecvData:
    rsz = co_await recv_stream(sd, buf = storage, 0, work);
    if (rsz == 0) // eof reached
        co_return;

    buf = {storage.data(), rsz};
SendData:
    ssz = co_await send_stream(sd, buf, 0, work);
    if (ssz == 0) // eof reached
        co_return;
    if (ssz == rsz) // send complete
        goto RecvData;

    // case: send size < recv size
    rsz -= ssz;
    buf = {storage.data() + ssz, rsz};
    goto SendData;
}
