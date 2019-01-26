//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>

TEST_CASE("resolver", "[network]")
{
    SECTION("host name")
    {
        const auto name = host_name();
        CAPTURE(name);
        REQUIRE(name);
    }

    SECTION("resolve for listen")
    {
        auto buffer = std::make_unique<char[]>(NI_MAXHOST);

        gsl::zstring<NI_MAXHOST> name = buffer.get();
        sockaddr_in6 ep{};
        ep.sin6_addr = in6addr_any;
        ep.sin6_port = htons(8080);

        // non-zero for error. redirect from getnameinfo
        auto ec = nameof(ep, name);
        if (ec == EAI_FAMILY)
        {
            WARN("ipv6 not supported");
            return;
        }

        REQUIRE(ec == 0);
    }

    SECTION("resolve for listen")
    {
        auto count = 0u;
        addrinfo hint{};
        hint.ai_flags
            = AI_PASSIVE | AI_V4MAPPED | AI_NUMERICSERV | AI_NUMERICHOST;
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        for (auto ep : resolve(hint, "0.0.0.0", "57132"))
        {
            REQUIRE(ep.sin6_family == AF_INET6);
            ++count;
        }
        REQUIRE(count > 0);
    }

    SECTION("resolve for connect")
    {
        auto count = 0u;
        addrinfo hint{};
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST;
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        for (auto ep : resolve(hint, "127.0.0.1", "echo"))
        {
            REQUIRE(ep.sin6_family == AF_INET6);
            ++count;
        }
        REQUIRE(count > 0);
    }
}
