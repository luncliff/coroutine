//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;
using namespace concrt;

using io_buffer_reserved_t = array<std::byte, 3900>;

auto test_recv_dgram(int64_t sd, int64_t& rsz, latch& wg) -> no_return {
    // count down the latch so `latch::wait` can return
    auto on_return = gsl::finally([&wg]() { wg.count_down(); });

    sockaddr_in remote{};
    io_work_t work{};               // struct to perform I/O request
    io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

    rsz = co_await recv_from(sd, remote, storage, work);
    // like `errno` or `WSAGetLastError`,  multiple read is ok
    if (auto ec = work.error()) {
        FAIL_WITH_CODE(ec);
        co_return;
    }
    REQUIRE(rsz > 0);
}

auto test_send_dgram(int64_t sd, const sockaddr_in& remote, int64_t& ssz,
                     latch& wg) -> no_return {
    // count down the latch so `latch::wait` can return
    auto on_return = gsl::finally([&wg]() { wg.count_down(); });

    io_work_t work{};               // struct to perform I/O request
    io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

    ssz = co_await send_to(sd, remote, storage, work);
    // like `errno` or `WSAGetLastError`,  multiple read is ok
    if (auto ec = work.error()) {
        FAIL_WITH_CODE(ec);
        co_return;
    }
    REQUIRE(static_cast<size_t>(ssz) == storage.size());
}

auto udp_echo_service(int64_t sd) -> no_return {
    int64_t rsz = 0, ssz = 0;
    sockaddr_in remote{};
    io_work_t work{};               // struct to perform I/O request
    io_buffer_t buf{};              // memory view to the `storage`
    io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

    while (true) {
        rsz = co_await recv_from(sd, remote, buf = storage, work);
        if (work.error())
            goto OnError;

        buf = {storage.data(), rsz};
        ssz = co_await send_to(sd, remote, buf, work);
        if (work.error())
            goto OnError;
    }
    co_return;
OnError:
    // expect ERROR_OPERATION_ABORTED (the socket is closed in this case)
    PRINT_MESSAGE(system_category().message(work.error()));
}

auto net_echo_udp_test() {
    init_network_api();
    auto on_return1 = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};    // address hint
    int64_t ss{};       // service int64_t
    endpoint_t local{}; // service endpoint

    static constexpr auto max_clients = 4;
    array<int64_t, max_clients> conns{};
    array<int64_t, max_clients> rsz{}, ssz{};

    auto on_return = gsl::finally([&ss, &conns]() {
        socket_close(ss);
        for (auto sd : conns)
            socket_close(sd);
        // this sleep is for waiting windows completion routines
        this_thread::sleep_for(1s);
    });

    // note
    //  we are using IPv4 here 
    //  since CI(docker) env doesn't support IPv6
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;
    ss = socket_create(hint);

    local.in4.sin_family = hint.ai_family;
    local.in4.sin_addr.s_addr = INADDR_ANY;
    local.in4.sin_port = htons(32771);
    socket_bind(ss, local);
    socket_set_option_nonblock(ss);
    udp_echo_service(ss);

    for (auto& sd : conns) {
        sd = socket_create(hint);

        endpoint_t ep = local;
        ep.in4.sin_port = 0; // let system define the port
        socket_bind(sd, ep);
        socket_set_option_nonblock(sd);
    }

    // We will spawn some coroutines and wait them to return using `latch`.
    // Those coroutines will perform send/recv operation on the socket
    latch wg{2 * max_clients};

    // local should hold service socket's port info
    local.in4.sin_port = htons(32771);
    for (auto i = 0U; i < max_clients; ++i) {
        test_recv_dgram(conns[i], rsz[i], wg);
        test_send_dgram(conns[i], local.in4, ssz[i], wg);
    }
    if constexpr (is_netinet) {
        while (wg.is_ready() == false)
            for (auto task : wait_io_tasks(2ms)) {
                task.resume();
            }
    }
    // wait for coroutines
    wg.wait();

    // now, receive coroutines must hold same data
    // sent by each client int64_ts
    for (auto i = 0U; i < max_clients; ++i) {
        // no i/o error
        REQUIRE(ssz[i] != -1);
        REQUIRE(rsz[i] != -1);
        // sent size == received size
        REQUIRE(ssz[i] == rsz[i]);
    }

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_echo_udp_test();
}

#elif __has_include(<CppUnitTest.h>)
class net_echo_udp : public TestClass<net_echo_udp> {
    TEST_METHOD(test_net_echo_udp) {
        net_echo_udp_test();
    }
};
#endif
