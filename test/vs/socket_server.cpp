// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <concrt.h>
#include <coroutine/net.h>
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <gsl/gsl>

#include <CppUnitTest.h>
#include <sdkddkver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using gsl::byte;
using gsl::finally;

void create_bound_udp_socket( //
    SOCKET& sd, sockaddr_in6& ep, const char* host, const char* port);
void create_bound_tcp_socket( //
    SOCKET& sd, sockaddr_in6& ep, const char* host, const char* port);

auto coro_send_to( //
    SOCKET sd, sockaddr_in6& remote, wait_group& wg) -> unplug;
auto coro_recv_from( //
    SOCKET sd, sockaddr_in6& remote, wait_group& wg) -> unplug;

auto coro_send_stream( //
    SOCKET sd, wait_group& wg) -> unplug;
auto coro_recv_stream( //
    SOCKET sd, wait_group& wg) -> unplug;

auto tcp_echo_service(SOCKET sd, wait_group& wg) -> unplug;
auto udp_echo_service(SOCKET sd, wait_group& wg) -> unplug;

class socket_async_server_test : public TestClass<socket_async_server_test>
{
    SOCKET ss, cs;         // server/client socket
    sockaddr_in6 sep, cep; // server/client endpoint
    int ec;

    TEST_METHOD_CLEANUP(close_all_socket)
    {
        closesocket(ss);
        closesocket(cs);
    }

    TEST_METHOD(socket_tcp_server)
    {
        create_bound_tcp_socket(ss, sep, "::", "61444");
        Assert::IsTrue(sep.sin6_port == htons(61444));
        ec = listen(ss, 1);
        Assert::IsTrue(ec == NO_ERROR);

        create_bound_tcp_socket(cs, cep, "::", "61443");
        Assert::IsTrue(cep.sin6_port == htons(61443));

        // connect to service's address
        sockaddr_in6 remote = sep;
        remote.sin6_addr = in6addr_loopback;

        connect(cs, (sockaddr*)&remote, sizeof(remote));
        Assert::IsTrue(ec == NO_ERROR);

        using namespace chrono_literals;

        auto accept_one_client = [](SOCKET ln, wait_group& wg) {
            SOCKET cs = accept(ln, nullptr, nullptr);
            Assert::IsTrue(cs != INVALID_SOCKET);

            tcp_echo_service(cs, wg); // attach echo coroutine to client
        };

        wait_group wg{};
        wg.add(3);

        // spawn an echo service and run client
        accept_one_client(ss, wg);
        coro_recv_stream(cs, wg);
        coro_send_stream(cs, wg);

        wg.wait(1s);
    }

    TEST_METHOD(socket_udp_server)
    {
        create_bound_udp_socket(ss, sep, "::", "61442");
        Assert::IsTrue(sep.sin6_port == htons(61442));

        create_bound_udp_socket(cs, cep, "::", "61441");
        Assert::IsTrue(cep.sin6_port == htons(61441));

        using namespace chrono_literals;
        // echo service's address
        sockaddr_in6 remote = sep;
        remote.sin6_addr = in6addr_loopback;

        wait_group wg{};
        wg.add(3);

        // spawn an echo service and run client
        udp_echo_service(ss, wg);
        coro_recv_from(cs, remote, wg);
        coro_send_to(cs, remote, wg);

        wg.wait(1s);
    }
};

auto tcp_echo_service(SOCKET sd, wait_group& wg) -> unplug
{
    // ensure noti to wait_group and close socket
    auto d1 = finally([&wg]() { wg.done(); });
    auto d2 = finally([=]() { closesocket(sd); });

    io_work_t work{};
    int64_t rsz = 0, ssz = 0;
    array<byte, 2000> storage{};

    do
    {
        rsz = co_await recv_stream(sd, storage, 0, work);
        Assert::IsTrue(rsz > 0);
        Assert::IsTrue(work.error() == NO_ERROR);

        ssz = co_await send_stream(sd, {storage.data(), rsz}, 0, work);
        Assert::IsTrue(ssz == rsz);
        Assert::IsTrue(work.error() == NO_ERROR);

    } while (false);
}

auto udp_echo_service(SOCKET sd, wait_group& wg) -> unplug
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    sockaddr_in6 remote{};
    int64_t rsz = 0, ssz = 0;
    array<byte, 2000> storage{};

    do
    {
        rsz = co_await recv_from(sd, remote, storage, work);
        Assert::IsTrue(rsz > 0);
        Assert::IsTrue(work.error() == NO_ERROR);

        ssz = co_await send_to(sd, remote, {storage.data(), rsz}, work);
        Assert::IsTrue(ssz == rsz);
        Assert::IsTrue(work.error() == NO_ERROR);

    } while (false);
}
