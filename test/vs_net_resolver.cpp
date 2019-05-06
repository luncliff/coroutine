//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/net.h>

#include <CppUnitTest.h>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern void print_error_message(int ec = WSAGetLastError());

class net_gethostname_test : public TestClass<net_gethostname_test> {

    TEST_METHOD(net_current_host_name) {
        const auto name = host_name();
        Assert::IsNotNull(name); // non-null

        Logger::WriteMessage(name);
    }
};

class net_getnameinfo_test : public TestClass<net_getnameinfo_test> {

    unique_ptr<char[]> name_buffer, serv_buffer;
    zstring_host name, serv;
    endpoint_t ep{};

    TEST_METHOD_INITIALIZE(allocate_string_buffers) {
        name_buffer = make_unique<char[]>(NI_MAXHOST);
        serv_buffer = make_unique<char[]>(NI_MAXSERV);
        name = name_buffer.get();
        serv = serv_buffer.get();
    }

    TEST_METHOD(net_get_name_ipv6_address) {

        sockaddr_in6& ip = ep.in6;
        ip.sin6_family = AF_INET6;
        ip.sin6_addr = in6addr_any;
        ip.sin6_port = htons(7654);

        // non-zero for error.
        // the value is redirected from `getnameinfo`
        if (auto ec = get_name(ep, name, nullptr)) {
            print_error_message();
            Assert::Fail();
        }
        // retry with service name buffer
        if (auto ec = get_name(ep, name, serv)) {
            print_error_message();
            Assert::Fail();
        }
    }

    TEST_METHOD(net_get_name_ipv4_address) {

        sockaddr_in& ip = ep.in4;
        ip.sin_family = AF_INET;
        ip.sin_addr.s_addr = INADDR_ANY;
        ip.sin_port = htons(7654);

        // non-zero for error.
        // the value is redirected from `getnameinfo`
        if (auto ec = get_name(ep, name, nullptr)) {
            print_error_message();
            Assert::Fail();
        }
        // retry with service name buffer
        if (auto ec = get_name(ep, name, serv)) {
            print_error_message();
            Assert::Fail();
        }
    }
};

class net_getaddrinfo_test : public TestClass<net_getaddrinfo_test> {

    addrinfo hint{};
    size_t count = 0u;

    TEST_METHOD(net_resolve_tcpv6_for_connect) {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST;

        for (auto ep : resolve(hint, "::1", "echo")) {
            Assert::IsTrue(ep.in6.sin6_family == AF_INET6);
            Assert::IsTrue(ep.in6.sin6_port == htons(7));
            auto loopback = IN6_IS_ADDR_LOOPBACK(&ep.in6.sin6_addr);
            Assert::IsTrue(loopback);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(net_resolve_tcpv6_for_listen_text) {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;

        for (auto ep : resolve(hint, nullptr, "https")) {
            Assert::IsTrue(ep.in6.sin6_family == AF_INET6);
            Assert::IsTrue(ep.in6.sin6_port == htons(443));
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(net_resolve_tcpv6_for_listen_numeric) {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags =
            AI_PASSIVE | AI_V4MAPPED | AI_NUMERICSERV | AI_NUMERICHOST;

        for (auto ep : resolve(hint, "fe80::", "57132")) {
            Assert::IsTrue(ep.in6.sin6_family == AF_INET6);
            Assert::IsTrue(ep.in6.sin6_port == htons(57132));
            // error message from macro is too long. make it short
            bool linklocal = IN6_IS_ADDR_LINKLOCAL(&ep.in6.sin6_addr);
            Assert::IsTrue(linklocal);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(net_resolve_udpv6_for_bind) {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::", "9283")) {
            Assert::IsTrue(ep.in6.sin6_family == AF_INET6);
            Assert::IsTrue(ep.in6.sin6_port == htons(9283));
            auto unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.in6.sin6_addr);
            Assert::IsTrue(unspec);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(net_resolve_ipv6_for_bind) {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_RAW;
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::0.0.0.0", "9287")) {
            Assert::IsTrue(ep.in6.sin6_family == AF_INET6);
            Assert::IsTrue(ep.in6.sin6_port == htons(9287));

            auto unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.in6.sin6_addr);
            Assert::IsTrue(unspec);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(net_resolve_ipv6_for_multicast) {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_RAW;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "FF0E::1", "7283")) {
            Assert::IsTrue(ep.in6.sin6_family == AF_INET6);
            Assert::IsTrue(ep.in6.sin6_port == htons(7283));

            auto global = IN6_IS_ADDR_MC_GLOBAL(&ep.in6.sin6_addr);
            Assert::IsTrue(global);

            ++count;
        }
        Assert::IsTrue(count > 0);
    }
};
