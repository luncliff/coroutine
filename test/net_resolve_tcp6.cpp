/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief get a list of address for IPv6, TCP connect
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

uint32_t resolve_tcp6_connect(addrinfo& hint) {
    hint.ai_flags = AI_ALL | AI_NUMERICHOST;

    size_t count = 0u;
    if (const auto ec = get_address(hint, "::1", "7", addresses)) {
        fputs(gai_strerror(ec), stderr);
        return ec;
    }

    for (const sockaddr_in6& ep : addresses) {
        assert(ep.sin6_family == AF_INET6);
        assert(ep.sin6_port == htons(7));
        bool loopback = IN6_IS_ADDR_LOOPBACK(addressof(ep.sin6_addr));
        assert(loopback);
        ++count;
    }
    return EXIT_SUCCESS;
}

uint32_t resolve_tcp6_listen_numeric(addrinfo& hint) {
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    if (const auto ec = get_address(hint, "fe80::", "57132", addresses)) {
        fputs(gai_strerror(ec), stderr);
        return ec;
    }

    for (const sockaddr_in6& ep : addresses) {
        assert(ep.sin6_family == AF_INET6);
        assert(ep.sin6_port == htons(57132));
        bool linklocal = IN6_IS_ADDR_LINKLOCAL(addressof(ep.sin6_addr));
        assert(linklocal);
        ++count;
    }
    assert(count > 0);
    return EXIT_SUCCESS;
}

uint32_t resolve_tcp6_listen_text(addrinfo& hint) {
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;

    size_t count = 0u;
    if (const auto ec = get_address(hint, nullptr, "https", addresses)) {
        fputs(gai_strerror(ec), stderr);
        return ec;
    }

    for (const sockaddr_in6& ep : addresses) {
        assert(ep.sin6_family == AF_INET6);
        assert(ep.sin6_port == htons(443));
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
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    if (auto ec = resolve_tcp6_connect(hint))
        return ec;
    if (auto ec = resolve_tcp6_listen_numeric(hint))
        return ec;
    if (auto ec = resolve_tcp6_listen_text(hint))
        return ec;
    return EXIT_SUCCESS;
}
