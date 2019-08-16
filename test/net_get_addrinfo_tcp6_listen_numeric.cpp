//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "socket.h"
#include <coroutine/net.h>

#include "test.h"
using namespace std;
using namespace coro;

auto net_getaddrinfo_tcp6_listen_numeric_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    for (sockaddr& ep : resolve(hint, "fe80::", "57132")) {
        _require_(ep.sa_family == AF_INET6);

        const auto* in6 = reinterpret_cast<sockaddr_in6*>(addressof(ep));
        _require_(in6->sin6_port == htons(57132));

        // error message from macro is too long. make it short
        bool linklocal = IN6_IS_ADDR_LINKLOCAL(addressof(in6->sin6_addr));
        _require_(linklocal);
        ++count;
    }
    _require_(count > 0);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_getaddrinfo_tcp6_listen_numeric_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class net_getaddrinfo_tcp6_listen_numeric
    : public TestClass<net_getaddrinfo_tcp6_listen_numeric> {
    TEST_METHOD(test_net_getaddrinfo_tcp6_listen_numeric) {
        net_getaddrinfo_tcp6_listen_numeric_test();
    }
};
#endif
