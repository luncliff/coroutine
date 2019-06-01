//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;

auto net_getnameinfo_v4_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    auto name_buffer = make_unique<char[]>(NI_MAXHOST);
    auto serv_buffer = make_unique<char[]>(NI_MAXSERV);
    zstring_host name = name_buffer.get();
    zstring_serv serv = serv_buffer.get();

    endpoint_t ep{};
    sockaddr_in& ip = ep.in4;
    ip.sin_family = AF_INET;
    ip.sin_addr.s_addr = INADDR_ANY;
    ip.sin_port = htons(7654);

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

#if __has_include(<CppUnitTest.h>)
class net_getnameinfo_v4 : public TestClass<net_getnameinfo_v4> {
    TEST_METHOD(test_net_getnameinfo_v4) {
        net_getnameinfo_v4_test();
    }
};
#else
int main(int, char* []) {
    return net_getnameinfo_v4_test();
}
#endif
