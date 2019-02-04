// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <gsl/gsl>

#include <CppUnitTest.h>
#include <sdkddkver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_MODULE_INITIALIZE(winsock_init)
{
    // https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-wsastartup
    WSAData wsa{};
    auto e = WSAGetLastError();
    Assert::IsTrue(e == NO_ERROR);

    e = WSAStartup(MAKEWORD(2, 2), &wsa);
    Assert::IsTrue(e == S_OK);
    Assert::IsTrue(e != WSASYSNOTREADY);
    Assert::IsTrue(e != WSAVERNOTSUPPORTED);
    Assert::IsTrue(e != WSAEFAULT);
}

TEST_MODULE_CLEANUP(winsock_clean)
{
    // https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-wsacleanup
    const auto e = WSACleanup();
    Assert::IsTrue(e != WSANOTINITIALISED);
    Assert::IsTrue(e != WSAENETDOWN);
    Assert::IsTrue(e != WSAEINPROGRESS);
}

class resolver_name_info_test : public TestClass<resolver_name_info_test>
{
    TEST_METHOD(get_name_current_host)
    {
        const auto name = host_name();
        Assert::IsNotNull(name); // non-null
    }

    TEST_METHOD(get_name_ipv6_address)
    {
        auto buffer = std::make_unique<char[]>(NI_MAXHOST);
        auto name = buffer.get();

        sockaddr_in6 ep{};
        ep.sin6_family = AF_INET6;
        ep.sin6_addr = in6addr_any;
        ep.sin6_port = htons(7654);

        // non-zero for error. it is redirected from getnameinfo
        auto ec = nameof(ep, name);
        Assert::IsTrue(ec == std::errc{});
    }

    TEST_METHOD(get_name_ipv4_address)
    {
        auto buffer = std::make_unique<char[]>(NI_MAXHOST);
        auto name = buffer.get();

        sockaddr_in ep{};
        ep.sin_family = AF_INET;
        ep.sin_addr.s_addr = INADDR_ANY;
        ep.sin_port = htons(7654);

        // non-zero for error. it is redirected from getnameinfo
        auto ec = nameof(ep, name);
        Assert::IsTrue(ec == std::errc{});
    }
};

class resolver_addr_info_test : public TestClass<resolver_addr_info_test>
{
    addrinfo hint{};
    size_t count = 0u;

    TEST_METHOD(get_addr_tcpv6_for_connect)
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_ALL | AI_V4MAPPED | AI_NUMERICHOST;

        for (auto ep : resolve(hint, "::1", "echo"))
        {
            Assert::IsTrue(ep.sin6_family == AF_INET6);
            Assert::IsTrue(ep.sin6_port == htons(7));
            auto loopback = IN6_IS_ADDR_LOOPBACK(&ep.sin6_addr);
            Assert::IsTrue(loopback);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(get_addr_tcpv6_for_listen_text)
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;

        hint.ai_flags = AI_PASSIVE | AI_V4MAPPED;
        for (auto ep : resolve(hint, nullptr, "https"))
        {
            Assert::IsTrue(ep.sin6_family == AF_INET6);
            Assert::IsTrue(ep.sin6_port == htons(443));
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(get_addr_tcpv6_for_listen_numeric)
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_PASSIVE | AI_V4MAPPED //
                        | AI_NUMERICSERV | AI_NUMERICHOST;

        for (auto ep : resolve(hint, "fe80::", "57132"))
        {
            Assert::IsTrue(ep.sin6_family == AF_INET6);
            Assert::IsTrue(ep.sin6_port == htons(57132));
            // error message from macro is too long. make it short
            bool linklocal = IN6_IS_ADDR_LINKLOCAL(&ep.sin6_addr);
            Assert::IsTrue(linklocal);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(get_addr_udpv6_for_bind)
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_flags = AI_ALL | AI_V4MAPPED //
                        | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::", "9283"))
        {
            Assert::IsTrue(ep.sin6_family == AF_INET6);
            Assert::IsTrue(ep.sin6_port == htons(9283));
            auto unspec = IN6_IS_ADDR_UNSPECIFIED(&ep.sin6_addr);
            Assert::IsTrue(unspec);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(get_addr_ipv6_for_bind)
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_RAW;
        hint.ai_flags = AI_ALL | AI_V4MAPPED //
                        | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "::0.0.0.0", "9287"))
        {
            Assert::IsTrue(ep.sin6_family == AF_INET6);
            Assert::IsTrue(ep.sin6_port == htons(9287));

            auto unspec= IN6_IS_ADDR_UNSPECIFIED(&ep.sin6_addr);
            Assert::IsTrue(unspec);
            ++count;
        }
        Assert::IsTrue(count > 0);
    }

    TEST_METHOD(get_addr_ipv6_for_multicast)
    {
        hint.ai_family = AF_INET6;
        hint.ai_socktype = SOCK_RAW;
        hint.ai_flags = AI_ALL | AI_NUMERICHOST | AI_NUMERICSERV;

        for (auto ep : resolve(hint, "FF0E::1", "7283"))
        {
            Assert::IsTrue(ep.sin6_family == AF_INET6);
            Assert::IsTrue(ep.sin6_port == htons(7283));

            auto global = IN6_IS_ADDR_MC_GLOBAL(&ep.sin6_addr);
            Assert::IsTrue(global);

            ++count;
        }
        Assert::IsTrue(count > 0);
    }
};
