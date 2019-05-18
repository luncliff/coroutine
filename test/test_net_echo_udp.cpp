//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;
using namespace concrt;

using io_buffer_reserved_t = array<std::byte, 3900>;

class net_echo_udp_test : public test_adapter {

    addrinfo hint{};    // address hint
    int64_t ss{};       // service int64_t
    endpoint_t local{}; // service endpoint

    static constexpr auto max_clients = 4;
    array<int64_t, max_clients> conns{};
    array<int64_t, max_clients> rsz{}, ssz{};
    array<endpoint_t, max_clients> remotes{};

  private:
    static auto test_recv_dgram(int64_t sd, int64_t& rsz, latch& wg)
        -> no_return {
        // count down the latch so `latch::wait` can return
        auto on_return = gsl::finally([&wg]() { wg.count_down(); });

        sockaddr_in6 remote{};
        io_work_t work{};               // struct to perform I/O request
        io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

        rsz = co_await recv_from(sd, remote, storage, work);
        expect_true(rsz > 0);
        expect_true(work.error() == 0);
    }
    static auto test_send_dgram(int64_t sd, const sockaddr_in6& remote,
                                int64_t& ssz, latch& wg) -> no_return {
        // count down the latch so `latch::wait` can return
        auto on_return = gsl::finally([&wg]() { wg.count_down(); });

        io_work_t work{};               // struct to perform I/O request
        io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

        ssz = co_await send_to(sd, remote, storage, work);
        expect_true(static_cast<size_t>(ssz) == storage.size());
        expect_true(work.error() == 0);
    }

    static auto udp_echo_service(int64_t sd) -> no_return {
        int64_t rsz = 0, ssz = 0;
        sockaddr_in6 remote{};
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
        print_message(system_category().message(work.error()));
    }

  public:
    void on_setup() override {
        // create service int64_t
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_protocol = IPPROTO_UDP;
        ss = socket_create(hint);
        if (ss < 0)
            fail_network_error();

        local.in6.sin6_family = hint.ai_family;
        local.in6.sin6_addr = in6addr_any;
        local.in6.sin6_port = htons(32771);
        socket_bind(ss, local);
        socket_set_option_nonblock(ss);
    }
    void on_teardown() override {
        socket_close(ss);
        for (auto sd : conns)
            socket_close(sd);
        // this sleep is for waiting windows completion routines
        this_thread::sleep_for(1s);
    }

    void on_test() override {
        udp_echo_service(ss);

        for (auto& sd : conns) {
            sd = socket_create(hint);

            endpoint_t ep = local;
            ep.in6.sin6_port = 0; // let system define the port
            socket_bind(sd, ep);
            socket_set_option_nonblock(sd);
        }

        // We will spawn some coroutines and wait them to return using `latch`.
        // Those coroutines will perform send/recv operation on the socket
        latch group{2 * max_clients};

        // local is holding service socket's port info
        local.in6.sin6_addr = in6addr_loopback;
        for (auto i = 0U; i < max_clients; ++i) {
            test_recv_dgram(conns[i], rsz[i], group);
            test_send_dgram(conns[i], local.in6, ssz[i], group);
        }
        if constexpr (is_netinet) {
            while (group.is_ready() == false)
                for (auto task : wait_io_tasks(2ms)) {
                    task.resume();
                }
        }
        // wait for coroutines
        group.wait();

        // now, receive coroutines must hold same data
        // sent by each client int64_ts
        for (auto i = 0U; i < max_clients; ++i) {
            // no i/o error
            expect_true(ssz[i] != -1);
            expect_true(rsz[i] != -1);
            // sent size == received size
            expect_true(ssz[i] == rsz[i]);
        }
    }
};
