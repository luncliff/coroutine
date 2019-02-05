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

void apply_nonblock_async(int64_t sd);
void create_bound_socket(int64_t& sd, sockaddr_in6& ep, const addrinfo& hint,
                         gsl::czstring<> host, gsl::czstring<> port);
void dispose_socket(int64_t sd);

auto create_bound_sockets(const addrinfo& hint, uint16_t port, size_t count)
    -> enumerable<int64_t>;

auto coro_recv_stream(int64_t sd, int64_t& rsz, wait_group& wg) -> unplug;
auto coro_send_stream(int64_t sd, int64_t& ssz, wait_group& wg) -> unplug;
auto echo_incoming_stream(int64_t sd) -> unplug;

void accept_dials(int64_t sd);

TEST_CASE("socket tcp echo test", "[network][socket][tcp]")
{
    int ec = 0;
    addrinfo hint{}; // address hint
    int64_t ln = -1; // listener socket
    endpoint_t ep{}; // service endpoint

    // create listener socket
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

    create_bound_socket(ln, ep.in6, hint, "::1", "32345");
    REQUIRE(ep.in6.sin6_port == htons(32345));

    // start listen
    ec = listen(ln, 7);
    REQUIRE(ec == 0);

    constexpr auto max_clients = 4;

    // wait group for coroutine sync
    wait_group wg{};
    // create some client sockets and data
    array<int64_t, max_clients> clients{};
    array<int64_t, max_clients> recv_lengths{};
    array<int64_t, max_clients> send_lengths{};

    gsl::index i = 0u;
    for (auto sd : create_bound_sockets(hint, 54251, clients.size()))
        clients[i++] = sd;

    // connect and option settings
    ep.in6.sin6_addr = in6addr_loopback;
    for (auto sd : clients)
    {
        connect(sd, addressof(ep.addr), sizeof(sockaddr_in6));
        CAPTURE(strerror(errno));

        std::printf("connect %ld %s\n", sd, strerror(errno));

        REQUIRE((errno == 0 || errno == EAGAIN || errno == EINPROGRESS));

        // no option settings for now ...
        apply_nonblock_async(sd);
    }

    //
    // we can run some background accept thread with
    //  `start_listen` and `stop_listen`, but
    //  we will just accept here to prevent complex debuging
    //  sourced from parallel execution of vstest
    //
    // apply_nonblock_async(ln);
    accept_dials(ln);
    std::printf("accept_dials done\n");

    // each client will perform 1 recv and 1 send
    wg.add(max_clients * 2);

    // recv packets. later echo response will resume the coroutines
    for (i = 0; i < max_clients; ++i)
        coro_recv_stream(clients[i], recv_lengths[i], wg);

    // send packets
    for (i = 0; i < max_clients; ++i)
        coro_send_stream(clients[i], send_lengths[i], wg);

    // !!!!! unlike windows api, we have to resume tasks manually !!!!!
    // the library doesn't guarantee they will be fetched at once
    // so user have to repeat enough
    {
        auto count = 30;
        while (count--)
            for (auto task : wait_io_tasks(0ms))
                task.resume();
    }

    // wait for coroutines
    wg.wait(10s);

    // now, receive coroutines must hold same data
    // sent by each client sockets
    for (i = 0; i < max_clients; ++i)
        REQUIRE(send_lengths[i] == recv_lengths[i]);

    // close
    for (auto sd : clients)
        dispose_socket(sd);

    // stop service
    dispose_socket(ln);
}

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

void accept_dials(int64_t ln)
{
    int64_t cs = -1;
    int ec = 0;

    auto count = 4;
    while (count--)
    {
        // notice that `ln` is non-blocking socket.
        cs = accept(ln, nullptr, nullptr);
        ec = errno;

        std::printf("cs %ld error %d %s\n", cs, ec, strerror(ec));
        // if (cs == -1) // accept failed
        // {
        //     auto em = std::system_category().message(ec);
        //     FAIL(em);
        //     return;
        // }

        echo_incoming_stream(cs);
    }
}

auto coro_recv_stream( //
    int64_t sd, int64_t& rsz, wait_group& wg) -> unplug
{
    using gsl::byte;

    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    array<byte, 2000> storage{};

    rsz = co_await recv_stream(sd, storage, 0, work);
    REQUIRE(work.error() == 0);
    REQUIRE(rsz > 0);
}

auto coro_send_stream( //
    int64_t sd, int64_t& ssz, wait_group& wg) -> unplug
{
    using gsl::byte;

    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    array<byte, 1523> storage{};

    ssz = co_await send_stream(sd, storage, 0, work);
    REQUIRE(work.error() == 0);
    REQUIRE(ssz > 0);

    shutdown(sd, SHUT_WR);
}

auto echo_incoming_stream(int64_t sd) -> unplug
{
    using gsl::byte;

    auto d = finally([=]() { dispose_socket(sd); });

    io_work_t work{};
    buffer_view_t buf{};
    int64_t rsz = 0, ssz = 0;
    array<byte, 3900> storage{};

    std::printf("client %ld %s\n", sd, __FUNCTION__);

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
