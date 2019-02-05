//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>

TEST_CASE("getnameinfo", "[network]")
{
    SECTION("host_name")
    {
        const auto name = host_name();
        CAPTURE(name);
        REQUIRE(name); // non-null
    }

    SECTION("nameof")
    {
        auto buffer = std::make_unique<char[]>(NI_MAXHOST);
        auto name = buffer.get(); // expect gsl::zstring<NI_MAXHOST>

        SECTION("ipv6")
        {
            sockaddr_in6 ep{};
            ep.sin6_family = AF_INET6;
            ep.sin6_addr = in6addr_any;
            ep.sin6_port = htons(7654);

            // non-zero for error. it is redirected from getnameinfo
            auto ec = nameof(ep, name);
            REQUIRE(ec == std::errc{});
        }
        SECTION("ipv4")
        {
            sockaddr_in ep{};
            ep.sin_family = AF_INET;
            ep.sin_addr.s_addr = INADDR_ANY;
            ep.sin_port = htons(7654);

            // non-zero for error. it is redirected from getnameinfo
            auto ec = nameof(ep, name);
            REQUIRE(ec == std::errc{});
        }
    }
}

TEST_CASE("getaddrinfo", "[network]")
{
    auto count = 0u;
    addrinfo hint{};

    SECTION("tcpv6")
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;

        SECTION("for listen")
        {
            hint.ai_flags = AI_PASSIVE | AI_V4MAPPED //
                            | AI_NUMERICSERV | AI_NUMERICHOST;
            for (auto ep : resolve(hint, "fe80::", "57132"))
            {
                REQUIRE(ep.sin6_family == AF_INET6);
                REQUIRE(ep.sin6_port == htons(57132));
                // error message from macro is too long. make it short
                bool linklocal = IN6_IS_ADDR_LINKLOCAL(&ep.sin6_addr);
                REQUIRE(linklocal);
                ++count;
            }
            REQUIRE(count > 0);
        }
        SECTION("for connect")
        {
            hint.ai_flags = AI_ALL | AI_V4MAPPED //
                            | AI_NUMERICHOST;
            for (auto ep : resolve(hint, "::1", "echo"))
            {
                REQUIRE(ep.sin6_family == AF_INET6);
                REQUIRE(ep.sin6_port == htons(7));
                auto loopback = IN6_IS_ADDR_LOOPBACK(&ep.sin6_addr);
                REQUIRE(loopback);
                ++count;
            }
            REQUIRE(count > 0);
        }
    }

    SECTION("udpv6")
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;

        SECTION("for bind")
        {
            hint.ai_flags = AI_ALL | AI_V4MAPPED //
                            | AI_NUMERICHOST | AI_NUMERICSERV;
            for (auto ep : resolve(hint, "::", "9283"))
            {
                REQUIRE(ep.sin6_family == AF_INET6);
                REQUIRE(ep.sin6_port == htons(9283));
                auto unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.sin6_addr);
                REQUIRE(unspec);
                ++count;
            }
            REQUIRE(count > 0);
        }

        SECTION("for bind v4mapped")
        {
            hint.ai_flags = AI_ALL | AI_V4MAPPED //
                            | AI_NUMERICHOST | AI_NUMERICSERV;
            for (auto ep : resolve(hint, "192.168.0.1", "9287"))
            {
                REQUIRE(ep.sin6_port == htons(9287));

                auto v4mapped = IN6_IS_ADDR_V4MAPPED(&ep.sin6_addr);
                REQUIRE(v4mapped);
                ++count;
            }
            REQUIRE(count > 0);
        }

        SECTION("for multicast")
        {
            // https://www.iana.org/assignments/ipv6-multicast-addresses/ipv6-multicast-addresses.xhtml
            hint.ai_flags = AI_ALL //
                            | AI_NUMERICHOST | AI_NUMERICSERV;
            for (auto ep : resolve(hint, "FF0E::1", "7283"))
            {
                REQUIRE(ep.sin6_family == AF_INET6);
                REQUIRE(ep.sin6_port == htons(7283));

                auto global = IN6_IS_ADDR_MC_GLOBAL(&ep.sin6_addr);
                REQUIRE(global);

                ++count;
            }
            REQUIRE(count > 0);
        }
    }
}
