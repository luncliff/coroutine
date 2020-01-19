/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief get a list of address for IPv6, TCP listen, with text `serv`
 */
#include <cassert>
#include <coroutine/net.h>

using namespace std;
using namespace coro;

void socket_setup() noexcept(false);
void socket_teardown() noexcept;

int main(int, char*[]) {
    socket_setup();
    auto on_return = gsl::finally([]() { socket_teardown(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;

    size_t count = 0u;
    generator<sockaddr_in6> addresses{};
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

#if __has_include(<WinSock2.h>)

WSADATA wsa{};

void socket_setup() noexcept(false) {
    if (::WSAStartup(MAKEWORD(2, 2), &wsa)) {
        throw system_error{WSAGetLastError(), system_category(), "WSAStartup"};
    }
}

void socket_teardown() noexcept {
    ::WSACleanup();
}

#elif __has_include(<netinet/in.h>)

void socket_setup() noexcept(false) {
    // do nothing for POSIX
}
void socket_teardown() noexcept {
    // do nothing for POSIX
}

#endif
