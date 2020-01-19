/**
 * @author github.com/luncliff <luncliff@gmail.com>
 * @brief Get a string representation from the `sockaddr_in6` object
 */
#include <coroutine/net.h>

using namespace std;
using namespace coro;

void socket_setup() noexcept(false);
void socket_teardown() noexcept;

int main(int, char*[]) {
    socket_setup();
    auto on_return = gsl::finally([]() { socket_teardown(); });

    auto name_buf = make_unique<char[]>(NI_MAXHOST);
    auto serv_buf = make_unique<char[]>(NI_MAXSERV);

    sockaddr_in6 in6{};
    in6.sin6_family = AF_INET6;
    in6.sin6_addr = in6addr_any;
    in6.sin6_port = htons(7654);

    // non-zero for error.
    // the value is redirected from `getnameinfo`
    if (auto ec = get_name(in6, name_buf.get(), nullptr)) {
        return ec;
    }
    // retry with service name buffer
    if (auto ec = get_name(in6, name_buf.get(), serv_buf.get())) {
        return ec;
    }
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
