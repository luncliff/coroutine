/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief get a list of address for IPv6, TCP listen, with text `serv`
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

int main(int, char*[]) {
    socket_setup();
    auto on_return = gsl::finally([]() { socket_teardown(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
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
