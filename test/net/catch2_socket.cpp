//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>
#include <gsl/gsl>

#include "./socket_test.h"

#if defined(_MSC_VER)
#include <Ws2tcpip.h>
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <netinet/tcp.h>
#endif

using namespace std;
using namespace gsl;

int64_t socket_create(const addrinfo& hint)
{
    int64_t sd = socket(hint.ai_family, hint.ai_socktype, hint.ai_protocol);
    if (sd == -1)
        FAIL(strerror(errno));
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
#if defined(_MSC_VER)
    shutdown(sd, SD_BOTH);
    closesocket(sd);
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    shutdown(sd, SHUT_RDWR);
    close(sd);
#endif
}

void socket_bind(int64_t sd, sockaddr_storage& storage)
{
    const auto family = storage.ss_family;
    socklen_t addrlen = sizeof(sockaddr_in);

    REQUIRE((family == AF_INET || family == AF_INET6));
    if (family == AF_INET6)
        addrlen = sizeof(sockaddr_in6);

    // bind socket and address
    if (::bind(sd, reinterpret_cast<sockaddr*>(&storage), addrlen))
        FAIL(strerror(errno));
}

void socket_listen(int64_t sd)
{
    if (listen(sd, 7) != 0)
        FAIL(strerror(errno));
}

void socket_set_option_nonblock(int64_t sd)
{
#if defined(_MSC_VER)
    u_long mode = TRUE;
    REQUIRE(ioctlsocket(sd, FIONBIO, &mode) == NO_ERROR);
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    // make non-block/async
    REQUIRE(fcntl(sd, F_SETFL, O_NONBLOCK) != -1);
#endif
}

void socket_set_option_reuse_address(int64_t sd)
{
    // reuse address for multiple test execution
    int opt = true;
    opt = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (opt != 0)
        FAIL(strerror(errno));
}

void socket_set_option_nodelay(int64_t sd)
{
    // reuse address for multiple test execution
    int opt = true;
    opt = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
    if (opt != 0)
        FAIL(strerror(errno));
}

#if defined(_MSC_VER)

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
    {
        auto errc = WSAGetLastError();
        auto em = system_category().message(errc);
        fputs(em.c_str(), stderr);
        // throw system_error{errc, system_category(), "WSAStartup"};
    }
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)

void load_network_api() noexcept(false)
{
    // do nothing for posix system. network operation already available
}

#endif
