//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "socket.h"
#include <coroutine/net.h>

#include "test.h"
using namespace std;
using namespace coro;

auto net_getnameinfo_v6_test() -> int {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    auto name_buffer = make_unique<char[]>(NI_MAXHOST);
    auto serv_buffer = make_unique<char[]>(NI_MAXSERV);
    zstring_host name = name_buffer.get();
    zstring_serv serv = serv_buffer.get();

    sockaddr_in6 in6{};
    in6.sin6_family = AF_INET6;
    in6.sin6_addr = in6addr_any;
    in6.sin6_port = htons(7654);

    // non-zero for error.
    // the value is redirected from `getnameinfo`
    if (auto ec = get_name(in6, name, nullptr)) {
        return __LINE__;
    }
    // retry with service name buffer
    if (auto ec = get_name(in6, name, serv)) {
        return __LINE__;
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_getnameinfo_v6_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class net_getnameinfo_v6 : public TestClass<net_getnameinfo_v6> {
    TEST_METHOD(test_net_getnameinfo_v6) {
        net_getnameinfo_v6_test();
    }
};
#endif
