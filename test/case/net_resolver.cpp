//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//

#include <catch2/catch.hpp>

#include <coroutine/enumerable.hpp>

#include <gsl/gsl>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using endpoint = sockaddr_in6;

auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>;
auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept -> enumerable<endpoint>;

uint32_t nameof(const endpoint& ep, //
                gsl::zstring<NI_MAXHOST> name) noexcept;
uint32_t nameof(const endpoint& ep, //
                gsl::zstring<NI_MAXHOST> name,
                gsl::zstring<NI_MAXSERV> serv) noexcept;

TEST_CASE("resolver")
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
        endpoint ep{};
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

char buf[NI_MAXHOST]{};
auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>
{
    ::gethostname(buf, NI_MAXHOST);
    return buf;
}

uint32_t nameof(const endpoint& ep, gsl::zstring<NI_MAXHOST> name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, nullptr,
                         0, flag);
}

uint32_t nameof(const endpoint& ep, //
                gsl::zstring<NI_MAXHOST> name,
                gsl::zstring<NI_MAXSERV> serv) noexcept
{
    //      NI_NAMEREQD
    //      NI_DGRAM
    //      NI_NOFQDN
    //      NI_NUMERICHOST
    //      NI_NUMERICSERV
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, serv,
                         NI_MAXSERV, flag);
}

auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept -> enumerable<endpoint>
{
    addrinfo* list = nullptr;

    if (auto ec = ::getaddrinfo(name, serv, //
                                std::addressof(hint), &list))
    {
        fputs(gai_strerror(ec), stderr);
        co_return;
    }

    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto d1 = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });

    for (addrinfo* iter = list; iter; iter = iter->ai_next)
    {
        endpoint& ep = *reinterpret_cast<endpoint*>(iter->ai_addr);
        // yield and proceed
        co_yield ep;
    }
}
