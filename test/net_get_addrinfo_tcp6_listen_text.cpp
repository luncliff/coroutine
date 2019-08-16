//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "socket.h"
#include <coroutine/net.h>

#include "test.h"
using namespace std;
using namespace coro;

auto net_getaddrinfo_tcp6_listen_text_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;

    size_t count = 0u;
    for (sockaddr& ep : resolve(hint, nullptr, "https")) {
        _require_(ep.sa_family == AF_INET6);
        const auto* in6 = reinterpret_cast<sockaddr_in6*>(addressof(ep));
        _require_(in6->sin6_port == htons(443));
        ++count;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_getaddrinfo_tcp6_listen_text_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class net_getaddrinfo_tcp6_listen_text
    : public TestClass<net_getaddrinfo_tcp6_listen_text> {
    TEST_METHOD(test_net_getaddrinfo_tcp6_listen_text) {
        net_getaddrinfo_tcp6_listen_text_test();
    }
};
#endif
