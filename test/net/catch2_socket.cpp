//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>
#include <coroutine/return.h>
#include <gsl/gsl>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace gsl;

void apply_nonblock_async(int64_t sd)
{
#if defined(_MSC_VER)
    u_long mode = TRUE;
    REQUIRE(ioctlsocket(sd, FIONBIO, &mode) == NO_ERROR);
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    // make non-block/async
    // REQUIRE(fcntl(sd, F_SETFL, O_NONBLOCK | O_ASYNC | fcntl(sd, F_GETFL, 0))
    //         != -1);
    REQUIRE(fcntl(sd, F_SETFL, O_NONBLOCK) != -1);
#endif
};

void create_bound_socket(int64_t& sd, sockaddr_in6& ep, const addrinfo& hint,
                         czstring<> host, czstring<> port)
{
    sd = socket(hint.ai_family, hint.ai_socktype, hint.ai_protocol);
    if (sd == -1)
        WARN(strerror(errno));

    REQUIRE(sd > 0);
    apply_nonblock_async(sd);

    // acquire address
    for (auto e : resolve(hint, host, port))
        ep = e;
    REQUIRE_FALSE(ep.sin6_port == 0);

    // bind socket and address
    REQUIRE(::bind(sd, reinterpret_cast<sockaddr*>(&ep), sizeof(ep)) == 0);
}

void dispose_socket(int64_t sd)
{
    shutdown(sd, SHUT_RDWR);
    close(sd);
}

auto create_bound_sockets(const addrinfo& hint, uint16_t port, size_t count)
    -> enumerable<int64_t>
{
    int64_t sd = -1;
    endpoint_t ep{};

    REQUIRE(hint.ai_family == AF_INET6);

    ep.in6.sin6_family = hint.ai_family;
    ep.in6.sin6_addr = in6addr_any;

    while (count--)
    {
        // use address hint in test class
        sd = socket(hint.ai_family, hint.ai_socktype, hint.ai_protocol);
        if (sd == -1)
            WARN(strerror(errno));

        REQUIRE(sd > 0);

        // datagram socket need to be bound to operate
        ep.in6.sin6_port = htons(port + count);

        // bind socket and address
        REQUIRE(::bind(sd, addressof(ep.addr), sizeof(sockaddr_in6)) == 0);

        co_yield sd;
    }
}