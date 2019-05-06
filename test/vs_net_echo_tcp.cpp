//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "net_test.h"
#include <coroutine/concrt.h>

#include <array>

#include <CppUnitTest.h>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace coro;
using namespace concrt;

extern void print_error_message(int ec = WSAGetLastError());

//  Recv a message and return without closing socket
//  `latch` is used to detect the coroutine is returned
auto test_recv_stream(SOCKET sd, int64_t& rsz, latch& wg) -> no_return;

//  Send a message and return without closing socket
//  `latch` is used to detect the coroutine is returned
auto test_send_stream(SOCKET sd, int64_t& ssz, latch& wg) -> no_return;

using on_accept_handler = no_return (*)(SOCKET);

//  Accept socket connects and invoke designated function
void accept_until_error(SOCKET ln, endpoint_t& remote,
                        on_accept_handler service);

//  Receive some blob from the socket and echo back the blob
//   return if the socket reaches EOF
auto tcp_echo_service(SOCKET sd) -> no_return;

class net_echo_tcp_test : public TestClass<net_echo_tcp_test> {

    addrinfo hint{};            // protocol/address hint
    SOCKET ln = INVALID_SOCKET; // listener socket
    endpoint_t local{};         // local: listening address

    // We will handle 4 connections concurrently
    static constexpr auto max_clients = 4U;
    array<SOCKET, max_clients> conns{}; // socket(connections) to the server
    array<int64_t, max_clients> rsz{};  // received data size
    array<int64_t, max_clients> ssz{};  // sent data size

    TEST_METHOD_INITIALIZE(setup) {
        // Create listener
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

        ln = socket_create(hint);

        local.addr.sa_family = (ADDRESS_FAMILY)hint.ai_family;
        local.in6.sin6_addr = in6addr_loopback;
        local.in6.sin6_port = htons(32345);

        socket_bind(ln, local);
        socket_set_option(ln, SOL_SOCKET, SO_REUSEADDR, true);
        // On Windows, non-block flag remains until the socket is closed
        socket_set_option_nonblock(ln);
    }

    TEST_METHOD_CLEANUP(teardown) {
        // close all known sockets
        for (auto sd : conns)
            socket_close(sd);
        socket_close(ln);
    }

    TEST_METHOD(serve_multiple_connections) {
        // Start listening. Also, now we know the server address
        socket_listen(ln);
        endpoint_t& server = local;

        // Make some dials to the server
        for (auto& sd : conns) {
            sd = socket_create(hint);
            if (sd == INVALID_SOCKET) {
                print_error_message();
                Assert::Fail(L"failed to create tcp socket");
            }
            if (auto ec = socket_connect(sd, server)) {
                print_error_message();
                Assert::Fail(L"failed to connect");
            }
        }

        // Accept as possible.
        // All accepted sockets will be served by the coroutine
        accept_until_error(ln, local, tcp_echo_service);

        //
        //  We will spawn some coroutines and wait them to return using `latch`.
        //  Those coroutines will perform send/recv operation on the socket
        //
        latch group{2 * max_clients};
        for (auto i = 0U; i < max_clients; ++i) {
            test_recv_stream(conns[i], rsz[i], group); // task 1: recv an echo
            test_send_stream(conns[i], ssz[i], group); // task 2: send a packet
        }
        group.wait();

        // Ok. Let's check the I/O was successful
        for (auto i = 0U; i < max_clients; ++i) {
            // no i/o error
            Assert::IsTrue(ssz[i] != -1);
            Assert::IsTrue(rsz[i] != -1);
            // sent size == received size
            Assert::IsTrue(ssz[i] == rsz[i]);
        }
    }
};

void accept_until_error(SOCKET ln, endpoint_t& remote,
                        on_accept_handler service) {

    Assert::IsTrue(service != nullptr);
    while (true) {
        auto cs = socket_accept(ln, remote);
        auto ec = WSAGetLastError();
        if (ec == EWOULDBLOCK) // it's non-block. try again
            continue;
        if (cs == INVALID_SOCKET) // accept failed.
                                  //  be verbose and break the loop
            return print_error_message();

        service(cs); // attach(spawn) a service coroutine
    }
}

auto tcp_echo_service(SOCKET sd) -> no_return {

    io_work_t work{};            // struct to perform I/O request
    io_buffer_t buf{};           // memory view to the `storage`
    int64_t rsz = 0;             // received data size
    int64_t ssz = 0;             // sent data size
    array<byte, 3900> storage{}; // each coroutine frame contains buffer

    // ensure close on coroutine frame's destruction
    auto on_return_disconnect = gsl::finally([=]() { socket_close(sd); });

RecvData:
    rsz = co_await recv_stream(sd, buf = storage, 0, work);
    if (rsz == 0) // eof reached
        co_return;
    buf = {storage.data(), rsz}; // buf := [base, rsz)

SendData:
    ssz = co_await send_stream(sd, buf, 0, work);
    if (ssz == 0) // eof reached
        co_return;
    if (ssz == rsz) // send complete
        goto RecvData;

    // send < recv : need to send more
    rsz -= ssz;
    buf = {storage.data() + ssz, rsz}; // buf := [base + ssz, rsz)
    goto SendData;
}

auto test_recv_stream(SOCKET sd, int64_t& rsz, latch& wg) -> no_return {

    io_work_t work{};            // struct to perform I/O request
    io_buffer_t buf{};           // memory view to the `storage`
    array<byte, 3900> storage{}; // each coroutine frame contains buffer

    // count down the latch so its `latch::wait` can return
    auto on_return = gsl::finally([&wg]() { wg.count_down(); });

    rsz = co_await recv_stream(sd, storage, 0, work);
    if (auto ec = work.error())
        print_error_message(ec);

    // like `errno` or `WSAGetLastError`,  multiple read is ok
    Assert::IsTrue(work.error() == NO_ERROR);
    Assert::IsTrue(rsz > 0);
}

auto test_send_stream(SOCKET sd, int64_t& ssz, latch& wg) -> no_return {

    io_work_t work{};            // struct to perform I/O request
    io_buffer_t buf{};           // memory view to the `storage`
    array<byte, 3900> storage{}; // each coroutine frame contains buffer

    // count down the latch so its `latch::wait` can return
    auto on_return = gsl::finally([&wg]() { wg.count_down(); });

    ssz = co_await send_stream(sd, storage, 0, work);
    if (auto ec = work.error())
        print_error_message(ec);

    // like `errno` or `WSAGetLastError`,  multiple read is ok
    Assert::IsTrue(work.error() == NO_ERROR);
    Assert::IsTrue(ssz > 0);
}
