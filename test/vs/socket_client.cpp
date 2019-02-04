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

class socket_async_client_test : public TestClass<socket_async_client_test>
{
    TEST_METHOD(socket_udp_client)
    {
        addrinfo hint{};
        sockaddr_in6 ep{};
        int ec;

        SOCKET sd = INVALID_SOCKET;
        create_bound_udp_socket(sd, ep, "::", "33421");
        Assert::IsTrue(ep.sin6_port == htons(33421));

        ep = sockaddr_in6{};
        // acquire echo server address
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICHOST;
        for (auto e : resolve(hint, "::1", "echo"))
            ep = e;
        Assert::IsTrue(ep.sin6_port == htons(7));
        using namespace chrono_literals;

        // trigger some async operations
        auto cnt = 5u;
        while (cnt--)
        {
            wait_group wg{};
            wg.add(2);

            // Regardless of system API support, this library doesn't guarantee
            //   parallel send or recv for a socket.
            // That is, the user *can* request recv and write at some timepoint,
            //   but at most 1 work is available for each operation
            coro_recv_from(sd, ep, wg);
            coro_send_to(sd, ep, wg);

            // spawning coroutines without synchronization
            //   will lead to undefined behavior
            wg.wait(1s);
        }
        closesocket(sd);
    }
};

void create_bound_tcp_socket( //
    SOCKET& sd, sockaddr_in6& ep, const char* host, const char* port)
{
    addrinfo hint{};
    auto ec = WSAGetLastError();

    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    // prepare socket
    sd = ::WSASocketW(hint.ai_family, hint.ai_socktype, hint.ai_protocol,
                      nullptr, 0, WSA_FLAG_OVERLAPPED);
    Assert::IsTrue(sd != INVALID_SOCKET);

    // bind to address
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICHOST | AI_NUMERICSERV;
    for (auto e : resolve(hint, host, port))
        ep = e;
    Assert::IsTrue(ep.sin6_port != 0);

    ec = ::bind(sd, (sockaddr*)&ep, sizeof(ep));
    Assert::IsTrue(ec == NO_ERROR);
}

auto coro_recv_stream( //
    SOCKET sd, wait_group& wg) -> unplug
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    int64_t rsz = 0;
    array<byte, 2000> storage{};

    rsz = co_await recv_stream(sd, storage, 0, work);
    Assert::IsTrue(rsz > 0);
    if (work.error() != NO_ERROR)
    {
        auto em = std::system_category().message(work.error());
        Logger::WriteMessage(em.c_str());
    }
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto coro_send_stream( //
    SOCKET sd, wait_group& wg) -> unplug
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    int64_t ssz = 0;
    array<byte, 2000> storage{};

    ssz = co_await send_stream(sd, storage, 0, work);
    Assert::IsTrue(ssz > 0);
    if (work.error() != NO_ERROR)
    {
        auto em = std::system_category().message(work.error());
        Logger::WriteMessage(em.c_str());
    }
    Assert::IsTrue(work.error() == NO_ERROR);
}

void create_bound_udp_socket( //
    SOCKET& sd, sockaddr_in6& ep, const char* host, const char* port)
{
    addrinfo hint{};
    auto ec = WSAGetLastError();

    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;

    // prepare socket
    sd = ::WSASocketW(hint.ai_family, hint.ai_socktype, hint.ai_protocol,
                      nullptr, 0, WSA_FLAG_OVERLAPPED);
    Assert::IsTrue(sd != INVALID_SOCKET);

    // bind to address
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICHOST | AI_NUMERICSERV;
    for (auto e : resolve(hint, host, port))
        ep = e;
    Assert::IsTrue(ep.sin6_port != 0);

    ec = ::bind(sd, (sockaddr*)&ep, sizeof(ep));
    Assert::IsTrue(ec == NO_ERROR);
}

auto coro_recv_from( //
    SOCKET sd, sockaddr_in6& remote, wait_group& wg) -> unplug
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    int64_t rsz = 0;
    array<byte, 2000> storage{};

    rsz = co_await recv_from(sd, remote, storage, work);
    Assert::IsTrue(rsz > 0);
    if (work.error() != NO_ERROR)
    {
        auto em = std::system_category().message(work.error());
        Logger::WriteMessage(em.c_str());
    }
    Assert::IsTrue(work.error() == NO_ERROR);
}

auto coro_send_to( //
    SOCKET sd, sockaddr_in6& remote, wait_group& wg) -> unplug
{
    // ensure noti to wait_group
    auto d = finally([&wg]() { wg.done(); });

    io_work_t work{};
    int64_t ssz = 0;
    array<byte, 2000> storage{};

    ssz = co_await send_to(sd, remote, storage, work);
    Assert::IsTrue(ssz > 0);
    if (work.error() != NO_ERROR)
    {
        auto em = std::system_category().message(work.error());
        Logger::WriteMessage(em.c_str());
    }
    Assert::IsTrue(work.error() == NO_ERROR);
}