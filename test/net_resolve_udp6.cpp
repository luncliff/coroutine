/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief get a list of address for IPv6, UDP unspecified
 */
#undef NDEBUG
#include <array>
#include <cassert>

#include <coroutine/net.h>

using namespace std;
using namespace coro;

// see 'external/sockets'
void socket_setup() noexcept(false);
void socket_teardown() noexcept;

array<sockaddr_in6, 1> addresses{};

uint32_t resolve_udp_unspecified(addrinfo& hint) {
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    if (const auto ec = get_address(hint, "::", "9283", addresses)) {
        fputs(gai_strerror(ec), stderr);
        return ec;
    }

    for (const sockaddr_in6& ep : addresses) {
        assert(ep.sin6_family == AF_INET6);
        assert(ep.sin6_port == htons(9283));
        bool unspec = IN6_IS_ADDR_UNSPECIFIED(addressof(ep.sin6_addr));
        assert(unspec);
        ++count;
    }
    assert(count > 0);
    return EXIT_SUCCESS;
}

uint32_t resolve_udp_v4mapped(addrinfo& hint) {
    hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    if (const auto ec = get_address(hint, //
                                    "::ffff:192.168.0.1", "9287", addresses)) {
        fputs(gai_strerror(ec), stderr);
        return ec;
    }

    for (const sockaddr_in6& ep : addresses) {
        assert(ep.sin6_family == AF_INET6);
        assert(ep.sin6_port == htons(9287));
        bool v4mapped = IN6_IS_ADDR_V4MAPPED(addressof(ep.sin6_addr));
        assert(v4mapped);
        ++count;
    }
    assert(count > 0);
    return EXIT_SUCCESS;
}

int main(int, char* []) {
    socket_setup();
    auto on_return = gsl::finally([]() { socket_teardown(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;

    if (auto ec = resolve_udp_unspecified(hint))
        return ec;
    if (auto ec = resolve_udp_v4mapped(hint))
        return ec;
    return EXIT_SUCCESS;
}
