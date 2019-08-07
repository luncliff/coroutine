//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/net.h>
#include "socket.h"

#include "test.h"
using namespace std;
using namespace coro;

auto net_getaddrinfo_udp6_bind_v4mapped_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    for (auto ep : resolve(hint, "::ffff:192.168.0.1", "9287")) {
        _require_(ep.in6.sin6_port == htons(9287));

        bool v4mapped = IN6_IS_ADDR_V4MAPPED(&ep.in6.sin6_addr);
        _require_(v4mapped);
        ++count;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_getaddrinfo_udp6_bind_v4mapped_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class net_getaddrinfo_udp6_bind_v4mapped
    : public TestClass<net_getaddrinfo_udp6_bind_v4mapped> {
    TEST_METHOD(test_net_getaddrinfo_udp6_bind_v4mapped) {
        net_getaddrinfo_udp6_bind_v4mapped_test();
    }
};
#endif
