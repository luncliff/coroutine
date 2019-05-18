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

void fail_network_error(int ec) {
    fail_with_message(system_category().message(ec));
}

int64_t socket_create(const addrinfo& hint) {
    int64_t sd = socket(hint.ai_family, hint.ai_socktype, hint.ai_protocol);
    if (sd == -1)
        fail_network_error();
    return sd;
}

socklen_t get_length(const endpoint_t& ep) noexcept {
    socklen_t len{};
    if (ep.storage.ss_family == AF_INET)
        len = sizeof(sockaddr_in);
    if (ep.storage.ss_family == AF_INET6)
        len = sizeof(sockaddr_in6);
    return len;
}

void socket_bind(int64_t sd, const endpoint_t& ep) {
    // bind socket and address
    if (::bind(sd, &ep.addr, get_length(ep)))
        fail_network_error();
}

void socket_listen(int64_t sd) {
    if (::listen(sd, 7) != 0)
        fail_network_error();
}

int64_t socket_connect(int64_t sd, const endpoint_t& remote) {
    return ::connect(sd, &remote.addr, get_length(remote));
}

int64_t socket_accept(int64_t ln) {
    return ::accept(ln, nullptr, nullptr);
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

int recent_net_error() noexcept {
    return WSAGetLastError();
}

bool is_in_progress(int ec) noexcept {
    return ec == WSAEWOULDBLOCK || ec == EWOULDBLOCK || ec == EINPROGRESS;
}
void socket_close(int64_t sd) {
    shutdown(sd, SD_BOTH);
    closesocket(sd);
}
void socket_set_option_nonblock(int64_t sd) {
    u_long mode = TRUE;
    expect_true(ioctlsocket(sd, FIONBIO, &mode) == NO_ERROR);
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
void init_network_api() noexcept(false) {
    // do nothing for posix system. network operation already available
}
void release_network_api() noexcept {
}

int recent_net_error() noexcept {
    return errno;
}

bool is_in_progress(int ec) noexcept {
    return ec == EINPROGRESS;
}
void socket_close(int64_t sd) {
    shutdown(sd, SHUT_RDWR);
    close(sd);
}
void socket_set_option_nonblock(int64_t sd) {
    // make non-block/async
    expect_true(fcntl(sd, F_SETFL, O_NONBLOCK) != -1);
}
#endif
