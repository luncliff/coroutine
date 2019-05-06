//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./net_test.h"

#include <CppUnitTest.h>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void print_error_message(int ec = WSAGetLastError()) {
    auto em = system_category().message(ec);
    Logger::WriteMessage(em.c_str());
}

// The library doesn't init/release Windows Socket API
//  Its user must do the work. In this case, the user is test code
WSADATA wsa_data{};

TEST_MODULE_INITIALIZE(winsock_init) {
    // init version the known version
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != NO_ERROR) {
        print_error_message();
        Assert::Fail();
    }
}

TEST_MODULE_CLEANUP(winsock_release) {
    // check and release
    if (wsa_data.wVersion != 0)
        ::WSACleanup();
}

int64_t socket_create(const addrinfo& hint) {

    auto sd = ::WSASocketW(hint.ai_family, hint.ai_socktype, hint.ai_protocol,
                           nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (sd == INVALID_SOCKET)
        print_error_message();

    Assert::AreNotEqual(sd, INVALID_SOCKET);
    return sd;
}

auto socket_create(const addrinfo& hint, //
                   size_t count) -> coro::enumerable<int64_t> {
    while (count--) {
        auto sd = socket_create(hint);
        co_yield sd;
    }
}

void socket_close(int64_t sd) {
    ::shutdown(sd, SD_BOTH);
    ::closesocket(sd);
}

void socket_bind(int64_t sd, endpoint_t& ep) {
    socklen_t addrlen = 0;

    const auto family = ep.storage.ss_family;
    if (family == AF_INET)
        addrlen = sizeof(sockaddr_in);
    if (family == AF_INET6)
        addrlen = sizeof(sockaddr_in6);

    // bind socket and address
    if (::bind(sd, &ep.addr, addrlen) != NO_ERROR) {
        print_error_message();
        Assert::Fail();
    }
}

void socket_listen(int64_t sd) {
    if (::listen(sd, 7) != NO_ERROR) {
        print_error_message();
        Assert::Fail();
    }
}

int64_t socket_connect(int64_t sd, const endpoint_t& remote) {
    socklen_t addrlen = 0;
    if (remote.addr.sa_family == AF_INET)
        addrlen = sizeof(remote.in4);
    if (remote.addr.sa_family == AF_INET6)
        addrlen = sizeof(remote.in6);

    return ::connect(sd, addressof(remote.addr), addrlen);
}

int64_t socket_accept(const int64_t ln, endpoint_t& ep) {
    // receiver can check the address family
    socklen_t addrlen = sizeof(ep.in6);
    return ::accept(ln, addressof(ep.addr), addressof(addrlen));
}

void socket_set_option_nonblock(int64_t sd) {
    u_long mode = TRUE;
    if (::ioctlsocket(sd, FIONBIO, &mode) != NO_ERROR) {
        print_error_message();
        Assert::Fail();
    }
}

void socket_set_option(int64_t sd, int64_t level, int64_t option,
                       int64_t value) {

    auto ec = ::setsockopt(sd, level, option, (char*)&value, sizeof(value));
    if (ec != NO_ERROR) {
        print_error_message();
        Assert::Fail();
    }
}

void socket_set_option_reuse_address(int64_t sd) {
    return socket_set_option(sd, SOL_SOCKET, SO_REUSEADDR, true);
}

void socket_set_option_nodelay(int64_t sd) {
    return socket_set_option(sd, IPPROTO_TCP, TCP_NODELAY, true);
}
