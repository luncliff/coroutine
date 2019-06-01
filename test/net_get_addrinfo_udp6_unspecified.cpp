//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;

auto net_getaddrinfo_udp6_bind_unspecified_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    for (auto ep : resolve(hint, "::", "9283")) {
        REQUIRE(ep.in6.sin6_family == AF_INET6);
        REQUIRE(ep.in6.sin6_port == htons(9283));

        bool unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.in6.sin6_addr);
        REQUIRE(unspec);
        ++count;
    }
    REQUIRE(count > 0);
    return EXIT_SUCCESS;
}

#if __has_include(<CppUnitTest.h>)
class net_getaddrinfo_udp6_bind_unspecified
    : public TestClass<net_getaddrinfo_udp6_bind_unspecified> {
    TEST_METHOD(test_net_getaddrinfo_udp6_bind_unspecified) {
        net_getaddrinfo_udp6_bind_unspecified_test();
    }
};
#else
int main(int, char* []) {
    return net_getaddrinfo_udp6_bind_unspecified_test();
}
#endif