/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 */
#include <cassert>
#include <cstdlib>

#include <coroutine/net.h>
#include <coroutine/return.h>

#include <socket.hpp>
#if defined(__APPLE__)
#include <latch_darwin.h>
#else
#include <latch.h>
#endif

using namespace std;
using namespace coro;

using io_buffer_reserved_t = array<std::byte, 3900>;

auto udp_recv_datagram(int64_t sd, io_work_t& work, //
                       int64_t& rsz, latch& wg) -> void {

    auto on_return = gsl::finally([&wg]() { wg.count_down(); });
    sockaddr_in remote{};
    io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

    rsz = co_await recv_from(sd, remote, storage, work);
    // like errno or WSAGetLastError,
    // using work.error() multiple read is ok
    if (auto ec = work.error()) {
        const auto emsg = system_category().message(ec);
        _fail_now_(emsg.c_str(), __FILE__, __LINE__);
    }
    _require_(rsz > 0);
}

auto udp_send_datagram(int64_t sd, io_work_t& work, //
                       const sockaddr_in& remote, int64_t& ssz, latch& wg)
    -> void {

    auto on_return = gsl::finally([&wg]() { wg.count_down(); });
    io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

    ssz = co_await send_to(sd, remote, storage, work);
    // like errno or WSAGetLastError,
    // using work.error() multiple read is ok
    if (auto ec = work.error()) {
        const auto emsg = system_category().message(ec);
        _fail_now_(emsg.c_str(), __FILE__, __LINE__);
    }
    _require_(static_cast<size_t>(ssz) == storage.size());
}

auto udp_echo_service(int64_t sd) -> void {
    sockaddr_in remote{};
    io_work_t work{};
    io_buffer_t buf{};              // memory view to the 'storage'
    io_buffer_reserved_t storage{}; // each coroutine frame contains buffer

    while (true) {
        // packet length(read)
        auto length = co_await recv_from(sd, remote, buf = storage, work);
        // instead of length check, see the error from the 'io_work_t' object
        if (work.error())
            goto OnError;

        buf = {storage.data(), length};
        length = co_await send_to(sd, remote, buf, work);
        if (work.error())
            goto OnError;

        _require_(length == buf.size_bytes());
    }
    co_return;
OnError:
    // expect ERROR_OPERATION_ABORTED (the socket is closed in this case)
    const auto ec = work.error();
    const auto emsg = system_category().message(ec);
    _println_(emsg.c_str());
}

int main(int, char* []) {
    socket_setup();
    auto on_return = gsl::finally([]() { socket_teardown(); });

    static constexpr auto max_socket_count = 4;
    static constexpr auto io_coroutine_count = max_socket_count * 2;

    // using IPv4 since CI(or Docker) env doesn't support IPv6
    addrinfo hint{};
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;

    // service socket
    int64_t ss = socket_create(hint);
    auto on_return2 = gsl::finally([ss]() {
        socket_close(ss);
        // this sleep is for waiting windows completion routines
        this_thread::sleep_for(1s);
    });

    sockaddr_in local{};
    local.sin_family = hint.ai_family;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(32771);
    socket_bind(ss, local);
    socket_set_option_nonblock(ss);

    // spawn echo coroutine
    udp_echo_service(ss);

    array<int64_t, max_socket_count> sockets{};
    array<int64_t, max_socket_count> rsz{}, ssz{}; // received/sent

    for (auto& sd : sockets) {
        sd = socket_create(hint);
        local.sin_port = 0; // let system define the port
        socket_bind(sd, local);
        socket_set_option_nonblock(sd);
        socket_set_option_timout(sd, 900);
    }
    auto on_return3 = gsl::finally([&sockets]() {
        for (auto sd : sockets)
            socket_close(sd);
    });

    // We should know where to send packets. Reuse the memory object
    sockaddr_in& remote = local;
    remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    remote.sin_port = htons(32771);

    // We will spawn some coroutines and wait them to return using `latch`.
    // Those coroutines will perform send/recv operation on the socket
    latch wg{2 * max_socket_count};

    // All I/O coroutines will use pre-allocated 'io_work_t'
    // so they can expose their last state which includes I/O error code
    array<io_work_t, io_coroutine_count> works{};

    for (auto i = 0U; i < max_socket_count; ++i) {
        udp_recv_datagram(sockets[i], works[2 * i + 0], rsz[i], wg);
        udp_send_datagram(sockets[i], works[2 * i + 1], remote, ssz[i], wg);
    }
    if constexpr (is_netinet) {
        // latch will help to sync the fork-join of coroutines
        while (wg.is_ready() == false) {
            for (auto task : wait_net_tasks(2ms))
                task.resume();
        }
    }
    wg.wait();

    // This is an echo. so receive/send length must be equal !
    for (auto i = 0U; i < max_socket_count; ++i) {
        _require_(ssz[i] != -1);     // no i/o error
        _require_(rsz[i] != -1);     //
        _require_(ssz[i] == rsz[i]); // sent == received
    }
    return EXIT_SUCCESS;
}
