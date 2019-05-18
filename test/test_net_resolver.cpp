//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;

class net_gethostname_test : public test_adapter {
  public:
    void on_test() override {
        const auto name = host_name();
        expect_true(name != nullptr);
    }
};

class net_getnameinfo_v4_test : public test_adapter {
    unique_ptr<char[]> name_buffer, serv_buffer;
    zstring_host name, serv;
    endpoint_t ep{};

  public:
    void on_setup() override {
        name_buffer = make_unique<char[]>(NI_MAXHOST);
        serv_buffer = make_unique<char[]>(NI_MAXSERV);
        name = name_buffer.get();
        serv = serv_buffer.get();
    }
    void on_test() override {
        sockaddr_in& ip = ep.in4;
        ip.sin_family = AF_INET;
        ip.sin_addr.s_addr = INADDR_ANY;
        ip.sin_port = htons(7654);

        // non-zero for error.
        // the value is redirected from `getnameinfo`
        if (auto ec = get_name(ep, name, nullptr)) {
            fail_with_message(system_category().message(ec));
            return;
        }
        // retry with service name buffer
        if (auto ec = get_name(ep, name, serv)) {
            fail_with_message(system_category().message(ec));
            return;
        }
    }
};

class net_getnameinfo_v6_test : public test_adapter {
    unique_ptr<char[]> name_buffer, serv_buffer;
    zstring_host name, serv;
    endpoint_t ep{};

  public:
    void on_setup() override {
        name_buffer = make_unique<char[]>(NI_MAXHOST);
        serv_buffer = make_unique<char[]>(NI_MAXSERV);
        name = name_buffer.get();
        serv = serv_buffer.get();
    }
    void on_test() override {

        sockaddr_in6& ip = ep.in6;
        ip.sin6_family = AF_INET6;
        ip.sin6_addr = in6addr_any;
        ip.sin6_port = htons(7654);

        // non-zero for error.
        // the value is redirected from `getnameinfo`
        if (auto ec = get_name(ep, name, nullptr)) {
            fail_with_message(system_category().message(ec));
            return;
        }
        // retry with service name buffer
        if (auto ec = get_name(ep, name, serv)) {
            fail_with_message(system_category().message(ec));
            return;
        }
    }
};

class net_getaddrinfo_tcp6_connect_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST;

        for (auto ep : resolve(hint, "::1", "echo")) {
            expect_true(ep.in6.sin6_family == AF_INET6);
            expect_true(ep.in6.sin6_port == htons(7));
            auto loopback = IN6_IS_ADDR_LOOPBACK(&ep.in6.sin6_addr);
            expect_true(loopback);
            ++count;
        }
        expect_true(count > 0);
    }
};

class net_getaddrinfo_tcp6_listen_text_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;

        for (auto ep : resolve(hint, nullptr, "https")) {
            expect_true(ep.in6.sin6_family == AF_INET6);
            expect_true(ep.in6.sin6_port == htons(443));
            ++count;
        }
        expect_true(count > 0);
    }
};

class net_getaddrinfo_tcp6_listen_numeric_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags =
            AI_PASSIVE | AI_V4MAPPED | AI_NUMERICSERV | AI_NUMERICHOST;

        for (auto ep : resolve(hint, "fe80::", "57132")) {
            expect_true(ep.in6.sin6_family == AF_INET6);
            expect_true(ep.in6.sin6_port == htons(57132));
            // error message from macro is too long. make it short
            bool linklocal = IN6_IS_ADDR_LINKLOCAL(&ep.in6.sin6_addr);
            expect_true(linklocal);
            ++count;
        }
        expect_true(count > 0);
    }
};

class net_getaddrinfo_udp6_bind_unspecified_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::", "9283")) {
            expect_true(ep.in6.sin6_family == AF_INET6);
            expect_true(ep.in6.sin6_port == htons(9283));
            auto unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.in6.sin6_addr);
            expect_true(unspec);
            ++count;
        }
        expect_true(count > 0);
    }
};

class net_getaddrinfo_udp6_bind_v4mapped_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::ffff:192.168.0.1", "9287")) {
            expect_true(ep.in6.sin6_port == htons(9287));

            auto v4mapped = IN6_IS_ADDR_V4MAPPED(&ep.in6.sin6_addr);
            expect_true(v4mapped);
            ++count;
        }
        expect_true(count > 0);
    }
};

class net_getaddrinfo_ip6_bind_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_RAW;
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::0.0.0.0", "9287")) {
            expect_true(ep.in6.sin6_family == AF_INET6);
            expect_true(ep.in6.sin6_port == htons(9287));

            auto unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.in6.sin6_addr);
            expect_true(unspec);
            ++count;
        }
        expect_true(count > 0);
    }
};

class net_getaddrinfo_for_multicast_test : public test_adapter {
    addrinfo hint{};
    size_t count = 0u;

  public:
    void on_test() override {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_RAW;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

        // https://www.iana.org/assignments/ipv6-multicast-addresses/ipv6-multicast-addresses.xhtml
        for (auto ep : resolve(hint, "FF0E::1", "7283")) {
            expect_true(ep.in6.sin6_family == AF_INET6);
            expect_true(ep.in6.sin6_port == htons(7283));

            auto global = IN6_IS_ADDR_MC_GLOBAL(&ep.in6.sin6_addr);
            expect_true(global);

            ++count;
        }
        expect_true(count > 0);
    }
};
