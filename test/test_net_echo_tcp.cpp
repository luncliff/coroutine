//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

#include <iostream>

using namespace coro;
using namespace concrt;

using on_accept_handler = auto (*)(int64_t) -> no_return;
using io_buffer_reserved_t = array<std::byte, 3900>;

class net_echo_tcp_test : public test_adapter {

    addrinfo hint{};    // protocol/address hint
    int64_t ln{};       // listener socket
    endpoint_t local{}; // local: listening address

    // We will handle 4 connections concurrently
    static constexpr auto max_clients = 4U;
    array<int64_t, max_clients> conns{}; // socket(connections) to the server
    array<int64_t, max_clients> rsz{};   // received data size
    array<int64_t, max_clients> ssz{};   // sent data size

  private:
    //  Accept socket connects and invoke designated function
    static void accept_until_error( //
        int64_t ln, gsl::not_null<on_accept_handler> service) {

        while (true) {
            // next accept is non-blocking
            socket_set_option_nonblock(ln);
            auto cs = socket_accept(ln);
            if (cs < 0) // accept failed
            {
                auto ec = recent_net_error();
                return print_message(system_category().message(ec));
            }
            socket_set_option_nonblock(cs); // set some options
            socket_set_option_nodelay(cs);
            service(cs); // attach(spawn) a service coroutine
        }
    }

    //  Receive some blob from the socket and echo back the blob
    //   return if the socket reaches EOF
    static auto tcp_echo_service(int64_t sd) -> no_return {
        // ensure close on coroutine frame's destruction
        auto on_return = gsl::finally([=]() { socket_close(sd); });

        int64_t rsz = 0, ssz = 0;       // received/sent data size
        io_work_t work{};               // struct to perform I/O request
        io_buffer_t buf{};              // memory view to the `storage`
        io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

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

    //  Recv a message and return without closing socket
    //  `latch` is used to detect the coroutine is returned
    static auto test_recv_stream(int64_t sd, int64_t& rsz, latch& wg)
        -> no_return {
        // count down the latch so `latch::wait` can return
        auto on_return = gsl::finally([&wg]() { wg.count_down(); });

        io_work_t work{};               // struct to perform I/O request
        io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

        rsz = co_await recv_stream(sd, storage, 0, work);
        if (auto ec = work.error())
            co_return fail_network_error(ec);

        // like `errno` or `WSAGetLastError`,  multiple read is ok
        expect_true(work.error() == 0);
        expect_true(rsz > 0);
    }

    //  Send a message and return without closing socket
    //  `latch` is used to detect the coroutine is returned
    static auto test_send_stream(int64_t sd, int64_t& ssz, latch& wg)
        -> no_return {
        // count down the latch so `latch::wait` can return
        auto on_return = gsl::finally([&wg]() { wg.count_down(); });

        io_work_t work{};               // struct to perform I/O request
        io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

        ssz = co_await send_stream(sd, storage, 0, work);
        if (auto ec = work.error())
            co_return fail_network_error(ec);

        // like `errno` or `WSAGetLastError`,  multiple read is ok
        expect_true(work.error() == 0);
        expect_true(ssz > 0);
    }

  public:
    void on_setup() override {
        // Create listener
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;
        ln = socket_create(hint);
        if (ln < 0)
            fail_network_error();

        local.addr.sa_family = hint.ai_family;
        local.in6.sin6_addr = in6addr_loopback;
        local.in6.sin6_port = htons(32345);
        socket_bind(ln, local);

        socket_set_option(ln, SOL_SOCKET, SO_REUSEADDR, true);
        socket_set_option_nonblock(ln);
    }
    void on_teardown() override {
        socket_close(ln);
        for (auto sd : conns)
            socket_close(sd);
        // this sleep is for waiting windows completion routines
        this_thread::sleep_for(1s);
    }

    void on_test() override {
        socket_listen(ln);

        // Make some dials to the server
        for (auto& sd : conns) {
            sd = socket_create(hint);
            if (sd < 0)
                return fail_network_error();
            // non-blocking connect
            socket_set_option_nonblock(sd);
            if (socket_connect(sd, local) < 0)
                // allow non-block error
                if (is_in_progress(recent_net_error()) == false)
                    return fail_network_error();
            // set some options ...
            socket_set_option_nodelay(sd);
        }

        // Accept as much as possible...
        // All accepted sockets will be served by the coroutine
        accept_until_error(ln, tcp_echo_service);

        // We will spawn some coroutines and wait them to return using `latch`.
        // Those coroutines will perform send/recv operation on the socket
        latch group{2 * max_clients};
        for (auto i = 0U; i < max_clients; ++i) {
            test_recv_stream(conns[i], rsz[i], group); // task 1: recv an echo
            test_send_stream(conns[i], ssz[i], group); // task 2: send a packet
        }
        if constexpr (is_netinet) {
            while (group.is_ready() == false)
                for (auto task : wait_io_tasks(4ms)) {
                    task.resume();
                }
        }
        group.wait();

        // check the I/O was successful
        for (auto i = 0U; i < max_clients; ++i) {
            // no i/o error
            expect_true(ssz[i] != -1);
            expect_true(rsz[i] != -1);
            // sent size == received size
            expect_true(ssz[i] == rsz[i]);
        }
    }
};
