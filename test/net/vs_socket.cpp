//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./socket_test.h"

#include <gsl/gsl>

#include <CppUnitTest.h>
#include <Ws2tcpip.h>
#include <sdkddkver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace gsl;

void create_bound_socket(SOCKET& sd, sockaddr_in6& ep, const addrinfo& hint,
                         gsl::czstring<> host, gsl::czstring<> port)
{
    // create socket
    sd = ::WSASocketW(hint.ai_family, hint.ai_socktype, hint.ai_protocol,
                      nullptr, 0, WSA_FLAG_OVERLAPPED);
    Assert::IsTrue(sd != INVALID_SOCKET);

    // acquire address
    for (auto e : resolve(hint, host, port))
        ep = e;
    Assert::IsTrue(ep.sin6_port != 0);

    // bind socket and address
    auto ec = ::bind(sd, reinterpret_cast<sockaddr*>(&ep), sizeof(ep));
    Assert::IsTrue(ec == NO_ERROR);
}

void fail_with_error_message(int errc)
{
    auto em = system_category().message(errc);
    auto wem = std::wstring{em.cbegin(), em.cend()};
    Assert::Fail(wem.c_str());
}

int64_t socket_create(const addrinfo& hint)
{
    // use address hint in test class
    int64_t sd = ::WSASocketW( //
        hint.ai_family, hint.ai_socktype, hint.ai_protocol, nullptr, 0,
        WSA_FLAG_OVERLAPPED);

    if (sd == INVALID_SOCKET)
        fail_with_error_message(WSAGetLastError());

    return sd;
}

auto socket_create(const addrinfo& hint, size_t count)
    -> coro::enumerable<int64_t>
{
    while (count--)
    {
        auto sd = socket_create(hint);
        co_yield sd;
    }
}

void socket_close(int64_t sd)
{
    shutdown(sd, SD_BOTH);
    closesocket(sd);
}

void socket_bind(int64_t sd, sockaddr_storage& storage)
{
    const auto family = storage.ss_family;
    socklen_t addrlen = sizeof(sockaddr_in);

    Assert::IsTrue((family == AF_INET || family == AF_INET6));
    if (family == AF_INET6)
        addrlen = sizeof(sockaddr_in6);

    // bind socket and address
    if (::bind(sd, reinterpret_cast<sockaddr*>(&storage), addrlen))
        fail_with_error_message(WSAGetLastError());
}

void socket_listen(int64_t sd)
{
    if (listen(sd, 7) != 0)
        fail_with_error_message(WSAGetLastError());
}

void socket_set_option_nonblock(int64_t sd)
{
    u_long mode = TRUE;
    Assert::IsTrue(ioctlsocket(sd, FIONBIO, &mode) == NO_ERROR);
}

void socket_set_option_reuse_address(int64_t sd)
{
    // reuse address for multiple test execution
    int opt = true;
    opt = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (opt != 0)
        fail_with_error_message(WSAGetLastError());
}

void socket_set_option_nodelay(int64_t sd)
{
    // reuse address for multiple test execution
    int opt = true;
    opt = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
    if (opt != 0)
        fail_with_error_message(WSAGetLastError());
}

WSADATA wsa_data{};
auto net_api_release = gsl::finally([]() noexcept {
    // check and release
    if (wsa_data.wVersion != 0)
        ::WSACleanup();

    wsa_data.wVersion = 0;
});

void load_network_api() noexcept(false)
{
    using namespace std;
    if (wsa_data.wVersion) // already initialized
        return;

    // init version 2.2
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data))
        fail_with_error_message(WSAGetLastError());
}
