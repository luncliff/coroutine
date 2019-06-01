//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;

auto net_getnameinfo_v6_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    auto name_buffer = make_unique<char[]>(NI_MAXHOST);
    auto serv_buffer = make_unique<char[]>(NI_MAXSERV);
    zstring_host name = name_buffer.get();
    zstring_serv serv = serv_buffer.get();

    endpoint_t ep{};
    sockaddr_in6& ip = ep.in6;
    ip.sin6_family = AF_INET6;
    ip.sin6_addr = in6addr_any;
    ip.sin6_port = htons(7654);

    // non-zero for error.
    // the value is redirected from `getnameinfo`
    if (auto ec = get_name(ep, name, nullptr)) {
        FAIL_WITH_CODE(ec);
        return EXIT_FAILURE;
    }
    // retry with service name buffer
    if (auto ec = get_name(ep, name, serv)) {
        FAIL_WITH_CODE(ec);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_getnameinfo_v6_test();
}

#elif __has_include(<CppUnitTest.h>)
class net_getnameinfo_v6 : public TestClass<net_getnameinfo_v6> {
    TEST_METHOD(test_net_getnameinfo_v6) {
        net_getnameinfo_v6_test();
    }
};
#endif
