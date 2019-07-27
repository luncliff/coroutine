//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "socket.h"
#include "test.h"

using namespace coro;

auto net_getaddrinfo_ip6_multicast_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    // https://www.iana.org/assignments/ipv6-multicast-addresses/ipv6-multicast-addresses.xhtml
    for (auto ep : resolve(hint, "FF0E::1", nullptr)) {
        _require_(ep.in6.sin6_family == AF_INET6);

        bool global = IN6_IS_ADDR_MC_GLOBAL(&ep.in6.sin6_addr);
        _require_(global);
        ++count;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_getaddrinfo_ip6_multicast_test();
}

#elif __has_include(<CppUnitTest.h>)
class net_getaddrinfo_ip6_multicast
    : public TestClass<net_getaddrinfo_ip6_multicast> {
    TEST_METHOD(test_net_getaddrinfo_ip6_multicast) {
        net_getaddrinfo_ip6_multicast_test();
    }
};
#endif
