//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;

auto net_gethostname_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    const auto name = host_name();
    REQUIRE(name != nullptr);
}

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
        return;
    }
    // retry with service name buffer
    if (auto ec = get_name(ep, name, serv)) {
        FAIL_WITH_CODE(ec);
        return;
    }
}

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
        return;
    }
    // retry with service name buffer
    if (auto ec = get_name(ep, name, serv)) {
        FAIL_WITH_CODE(ec);
        return;
    }
}

auto net_getaddrinfo_tcp6_connect_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ALL | AI_NUMERICHOST;

    size_t count = 0u;
    for (auto ep : resolve(hint, "::1", "echo")) {
        REQUIRE(ep.in6.sin6_family == AF_INET6);
        REQUIRE(ep.in6.sin6_port == htons(7));
        auto loopback = IN6_IS_ADDR_LOOPBACK(&ep.in6.sin6_addr);
        REQUIRE(loopback);
        ++count;
    }
    REQUIRE(count > 0);
}

auto net_getaddrinfo_tcp6_listen_text_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;

    size_t count = 0u;
    for (auto ep : resolve(hint, nullptr, "https")) {
        REQUIRE(ep.in6.sin6_family == AF_INET6);
        REQUIRE(ep.in6.sin6_port == htons(443));
        ++count;
    }
    REQUIRE(count > 0);
}

auto net_getaddrinfo_tcp6_listen_numeric_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE | AI_V4MAPPED | AI_NUMERICSERV | AI_NUMERICHOST;

    size_t count = 0u;
    for (auto ep : resolve(hint, "fe80::", "57132")) {
        REQUIRE(ep.in6.sin6_family == AF_INET6);
        REQUIRE(ep.in6.sin6_port == htons(57132));

        // error message from macro is too long. make it short
        bool linklocal = IN6_IS_ADDR_LINKLOCAL(&ep.in6.sin6_addr);
        REQUIRE(linklocal);
        ++count;
    }
    REQUIRE(count > 0);
}

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
}

auto net_getaddrinfo_udp6_bind_v4mapped_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    for (auto ep : resolve(hint, "::ffff:192.168.0.1", "9287")) {
        REQUIRE(ep.in6.sin6_port == htons(9287));

        bool v4mapped = IN6_IS_ADDR_V4MAPPED(&ep.in6.sin6_addr);
        REQUIRE(v4mapped);
        ++count;
    }
    REQUIRE(count > 0);
}

auto net_getaddrinfo_ip6_bind_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    addrinfo hint{};
    hint.ai_family = AF_INET6;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

    size_t count = 0u;
    // since this is ipv6, ignore port(service) number
    for (auto ep : resolve(hint, "::0.0.0.0", nullptr)) {
        REQUIRE(ep.in6.sin6_family == AF_INET6);

        bool unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.in6.sin6_addr);
        REQUIRE(unspec);
        ++count;
    }
    REQUIRE(count > 0);
}

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
        REQUIRE(ep.in6.sin6_family == AF_INET6);

        bool global = IN6_IS_ADDR_MC_GLOBAL(&ep.in6.sin6_addr);
        REQUIRE(global);
        ++count;
    }
    REQUIRE(count > 0);
}

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("current host name", "[network]") {
    net_gethostname_test();
}
TEST_CASE("getnameinfo ipv4", "[network]") {
    net_getnameinfo_v4_test();
}
TEST_CASE("getnameinfo ipv6", "[network]") {
    net_getnameinfo_v6_test();
}
TEST_CASE("getaddrinfo tcp6 connect", "[network]") {
    net_getaddrinfo_tcp6_connect_test();
}
TEST_CASE("getaddrinfo tcp6 listen text", "[network]") {
    net_getaddrinfo_tcp6_listen_text_test();
}
TEST_CASE("getaddrinfo tcp6 listen numeric", "[network]") {
    net_getaddrinfo_tcp6_listen_numeric_test();
}
TEST_CASE("getaddrinfo udp6 bind unspecified", "[network]") {
    net_getaddrinfo_udp6_bind_unspecified_test();
}
TEST_CASE("getaddrinfo udp6 bind v4mapped", "[network]") {
    net_getaddrinfo_udp6_bind_v4mapped_test();
}
TEST_CASE("getaddrinfo ip6 bind", "[network]") {
    net_getaddrinfo_ip6_bind_test();
}
TEST_CASE("getaddrinfo ip6 multicast", "[network]") {
    net_getaddrinfo_ip6_multicast_test();
}

#elif __has_include(<CppUnitTest.h>)

class net_gethostname : public TestClass<net_gethostname> {
    TEST_METHOD(test_net_gethostname) {
        net_gethostname_test();
    }
};
class net_getnameinfo_v4 : public TestClass<net_getnameinfo_v4> {
    TEST_METHOD(test_net_getnameinfo_v4) {
        net_getnameinfo_v4_test();
    }
};
class net_getnameinfo_v6 : public TestClass<net_getnameinfo_v6> {
    TEST_METHOD(test_net_getnameinfo_v6) {
        net_getnameinfo_v6_test();
    }
};
class net_getaddrinfo_tcp6_connect
    : public TestClass<net_getaddrinfo_tcp6_connect> {
    TEST_METHOD(test_net_getaddrinfo_tcp6_connect) {
        net_getaddrinfo_tcp6_connect_test();
    }
};
class net_getaddrinfo_tcp6_listen_text
    : public TestClass<net_getaddrinfo_tcp6_listen_text> {
    TEST_METHOD(test_net_getaddrinfo_tcp6_listen_text) {
        net_getaddrinfo_tcp6_listen_text_test();
    }
};
class net_getaddrinfo_tcp6_listen_numeric
    : public TestClass<net_getaddrinfo_tcp6_listen_numeric> {
    TEST_METHOD(test_net_getaddrinfo_tcp6_listen_numeric) {
        net_getaddrinfo_tcp6_listen_numeric_test();
    }
};
class net_getaddrinfo_udp6_bind_unspecified
    : public TestClass<net_getaddrinfo_udp6_bind_unspecified> {
    TEST_METHOD(test_net_getaddrinfo_udp6_bind_unspecified) {
        net_getaddrinfo_udp6_bind_unspecified_test();
    }
};
class net_getaddrinfo_udp6_bind_v4mapped
    : public TestClass<net_getaddrinfo_udp6_bind_v4mapped> {
    TEST_METHOD(test_net_getaddrinfo_udp6_bind_v4mapped) {
        net_getaddrinfo_udp6_bind_v4mapped_test();
    }
};
class net_getaddrinfo_ip6_bind : public TestClass<net_getaddrinfo_ip6_bind> {
    TEST_METHOD(test_net_getaddrinfo_ip6_bind) {
        net_getaddrinfo_ip6_bind_test();
    }
};
class net_getaddrinfo_ip6_multicast
    : public TestClass<net_getaddrinfo_ip6_multicast> {
    TEST_METHOD(test_net_getaddrinfo_ip6_multicast) {
        net_getaddrinfo_ip6_multicast_test();
    }
};
#endif
