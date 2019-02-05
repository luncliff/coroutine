//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include "./socket.h"

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

auto socket_create(const addrinfo& hint, size_t count) -> enumerable<int64_t>
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

void socket_bind(int64_t sd, sockaddr_in6& ep)
{
    // bind socket and address
    if (::bind(sd, reinterpret_cast<sockaddr*>(&ep), sizeof(ep)) != 0)
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
    REQUIRE(fcntl(sd, F_SETFL, O_NONBLOCK | O_ASYNC | fcntl(sd, F_GETFL, 0))
            != -1);
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
