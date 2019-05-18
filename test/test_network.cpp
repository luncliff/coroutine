//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//

#if __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>
#else
#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>
#endif

#include "test_network.h"
#include "test_shared.h"

using namespace std;

#if defined(_MSC_VER) || defined(_WINDOWS_)
void fail_network_error(int ec = WSAGetLastError());
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
void fail_network_error(int ec = errno);
#endif

void fail_network_error(int ec) {
    fail_with_message(system_category().message(ec));
}

int64_t socket_create(const addrinfo& hint) {
    int64_t sd = socket(hint.ai_family, hint.ai_socktype, hint.ai_protocol);
    if (sd == -1)
        fail_network_error();
    return sd;
}

auto socket_create(const addrinfo& hint, size_t count)
    -> coro::enumerable<int64_t> {
    while (count--) {
        auto sd = socket_create(hint);
        co_yield sd;
    }
}

void socket_close(int64_t sd) {
#if defined(_MSC_VER) || defined(_WINDOWS_)
    shutdown(sd, SD_BOTH);
    closesocket(sd);
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    shutdown(sd, SHUT_RDWR);
    close(sd);
#endif
}

void socket_bind(int64_t sd, endpoint_t& ep) {
    socklen_t addrlen = sizeof(sockaddr_in);
    if (ep.storage.ss_family == AF_INET)
        addrlen = sizeof(sockaddr_in);
    if (ep.storage.ss_family == AF_INET6)
        addrlen = sizeof(sockaddr_in6);

    // bind socket and address
    if (::bind(sd, &ep.addr, addrlen))
        fail_network_error();
}

void socket_listen(int64_t sd) {
    if (::listen(sd, 7) != 0)
        fail_network_error();
}

void socket_set_option_nonblock(int64_t sd) {
#if defined(_MSC_VER)
    u_long mode = TRUE;
    expect_true(ioctlsocket(sd, FIONBIO, &mode) == NO_ERROR);
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    // make non-block/async
    REQUIRE(fcntl(sd, F_SETFL, O_NONBLOCK) != -1);
#endif
}

void socket_set_option(int64_t sd, int64_t level, int64_t option,
                       int64_t value) {

    auto ec = ::setsockopt(sd, level, option, (char*)&value, sizeof(value));
    if (ec != 0)
        fail_network_error();
}

void socket_set_option_reuse_address(int64_t sd) {
    return socket_set_option(sd, SOL_SOCKET, SO_REUSEADDR, true);
}

void socket_set_option_nodelay(int64_t sd) {
    return socket_set_option(sd, IPPROTO_TCP, TCP_NODELAY, true);
}

#if defined(_MSC_VER)

WSADATA wsa_data{};

void init_network_api() noexcept(false) {
    if (wsa_data.wVersion) // already initialized
        return;

    // init version 2.2
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
        auto errc = WSAGetLastError();
        throw system_error{errc, system_category(), "WSAStartup"};
    }
}
void release_network_api() noexcept {
    // check and release
    if (wsa_data.wVersion != 0)
        ::WSACleanup();

    wsa_data.wVersion = 0;
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
void init_network_api() noexcept(false) {
    // do nothing for posix system. network operation already available
}
void release_network_api() noexcept {
}
#endif
