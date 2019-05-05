//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/concrt.h>
#include <coroutine/net.h>
#include <coroutine/return.h>
#include <gsl/gsl>

#include "./socket_test.h"

using namespace std;
using namespace std::chrono_literals;
using namespace gsl;
using namespace coro;

using wait_group = concrt::latch;

auto coro_recv_stream(int64_t sd, int64_t& rsz, wait_group& wg) -> no_return;
auto coro_send_stream(int64_t sd, int64_t& ssz, wait_group& wg) -> no_return;
auto echo_incoming_stream(int64_t sd) -> no_return;

TEST_CASE("socket tcp echo test", "[network][socket]") {
    load_network_api();

    constexpr auto test_listen_port = 32345;

    addrinfo hint{};
    hint.ai_family = AF_INET; // test with ipv4
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    int64_t ln = socket_create(hint);
    auto d1 = gsl::finally([ln]() { socket_close(ln); });

    socket_set_option_reuse_address(ln);

    endpoint_t ep{};
    ep.in6.sin6_family = hint.ai_family;        //   -- ipv6 --
    ep.in4.sin_addr.s_addr = htonl(INADDR_ANY); // in6.sin6_addr <- in6addr_any
    ep.in4.sin_port = htons(test_listen_port);  // in6.sin6_port <- htons(port)

    socket_bind(ln, ep);
    socket_listen(ln);

    SECTION("multiple clients") {
        constexpr auto max_clients = 4;

        // create some client sockets and data
        array<int64_t, max_clients> clients{};
        array<int64_t, max_clients> recv_lengths{};
        array<int64_t, max_clients> send_lengths{};

        gsl::index i = 0u;
        for (auto sd : socket_create(hint, clients.size())) {
            clients[i++] = sd;
            socket_set_option_nonblock(sd);
        }
        auto d2 = gsl::finally([&clients]() {
            for (auto sd : clients)
                socket_close(sd);
        });

        // connect to the service
        // in6.sin6_addr <- in6addr_loopback; // for ipv6
        ep.in4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        for (auto sd : clients) {
            // be cautious aboud addrlen
            connect(sd, addressof(ep.addr), sizeof(sockaddr_in));
            CAPTURE(errno);
            CAPTURE(strerror(errno));
            REQUIRE((errno == EINPROGRESS || errno == 0));
            socket_set_option_nodelay(sd);
        }

        //
        // we can run some background accept thread with
        //  `start_listen` and `stop_listen`, but
        //  we will just accept here to prevent complex debuging
        //  sourced from parallel execution of vstest
        //
        while (true) {
            socket_set_option_nonblock(ln); // next accept is non-blocking

            auto cs = accept(ln, nullptr, nullptr);
            if (cs == -1) // accept failed
                break;

            socket_set_option_nonblock(cs); // set some options
            socket_set_option_nodelay(cs);  // and then
            echo_incoming_stream(cs);       // attach service coroutine
        }

        wait_group wg{max_clients *
                      2}; // wait group for coroutine sync
                          // each client will perform 1 recv and 1 send
        {
            // recv packets. later echo response will resume the coroutines
            for (i = 0; i < max_clients; ++i)
                coro_recv_stream(clients[i], recv_lengths[i], wg);

            // send packets
            for (i = 0; i < max_clients; ++i)
                coro_send_stream(clients[i], send_lengths[i], wg);

            if constexpr (is_winsock == false) {
                // unlike windows api, we have to resume tasks manually
                // the library doesn't guarantee they will be fetched at once
                // so user have to repeat enough to finish all i/o tasks
                auto count = 30;
                while (count--) {
                    for (auto task : wait_io_tasks(10ms))
                        task.resume();
                }
            }
        }
        wg.wait(); // ensure all coroutines are finished

        // now, receive coroutines must hold same data
        // sent by each client sockets
        for (i = 0; i < max_clients; ++i)
            REQUIRE(send_lengths[i] == recv_lengths[i]);
    }
    // test end
}

auto coro_recv_stream( //
    int64_t sd, int64_t& rsz, wait_group& wg) -> no_return {
    using gsl::byte;
    auto d = finally([&wg]() { // ensure noti to wait_group
        wg.count_down();
    });

    io_work_t work{};
    array<byte, 2000> storage{};

    rsz = co_await recv_stream(sd, storage, 0, work);
    if (auto errc = work.error()) {
        CAPTURE(errc);
        FAIL(std::system_category().message(errc));
    }
    REQUIRE(rsz > 0);
}

auto coro_send_stream(int64_t sd, int64_t& ssz, wait_group& wg) -> no_return {
    using gsl::byte;

    auto d = finally([&wg]() { // ensure noti to wait_group
        wg.count_down();
    });

    io_work_t work{};
    array<byte, 1523> storage{};

    ssz = co_await send_stream(sd, storage, 0, work);
    if (auto errc = work.error()) {
        CAPTURE(errc);
        FAIL(std::system_category().message(errc));
    }
    REQUIRE(ssz > 0);
}

auto echo_incoming_stream(int64_t sd) -> no_return {
    using gsl::byte;

    auto d = finally([=]() { socket_close(sd); });

    io_work_t work{};
    io_buffer_t buf{};
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
