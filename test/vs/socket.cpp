// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

#include "./vstest.h"
#include <coroutine/net.h>

#include <CppUnitTest.h>
#include <sdkddkver.h>

using namespace std::literals;

class SocketTest : public TestClass<SocketTest>
{
    using error_t = uint32_t;

    // https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-wsastartup
    TEST_CLASS_INITIALIZE(Setup)
    {
        error_t e{};
        WSAData wsa{};

        e = WSAGetLastError();
        Assert::IsTrue(e == NO_ERROR);

        e = WSAStartup(MAKEWORD(2, 2), &wsa);
        Assert::IsTrue(e == S_OK);
        Assert::IsTrue(e != WSASYSNOTREADY);
        Assert::IsTrue(e != WSAVERNOTSUPPORTED);
        Assert::IsTrue(e != WSAEFAULT);
    }

    // https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-wsacleanup
    TEST_CLASS_CLEANUP(TearDown)
    {
        error_t e{};

        e = WSACleanup();
        Assert::IsTrue(e != WSANOTINITIALISED);
        Assert::IsTrue(e != WSAENETDOWN);
        Assert::IsTrue(e != WSAEINPROGRESS);
    }

    TEST_METHOD(GetSetError)
    {
        error_t e{};

        e = WSAGetLastError();
        Assert::IsTrue(e == S_OK);
        WSASetLastError(WSANOTINITIALISED);

        e = WSAGetLastError();
        Assert::IsTrue(e == WSANOTINITIALISED);
    }

    static addrinfo make_hint(int flags, int family, int sock) noexcept
    {
        addrinfo h{};
        h.ai_flags = flags;
        h.ai_family = family;
        h.ai_socktype = sock;
        return h;
    }

    TEST_METHOD(ResolveToConnect)
    {
        error_t e{};
        // char name[NI_MAXHOST]{};
        // char serv[NI_MAXSERV]{};
        const char* name = "www.google.com";
        const char* serv = "https";

        // for `connect()`    // TCP + IPv6
        addrinfo hint = make_hint(AI_ALL | AI_V4MAPPED, AF_INET6, SOCK_STREAM);
        addrinfo* list = nullptr;

        // Success : zero
        // Failure : non-zero uint32_t code
        e = ::getaddrinfo(name, serv, &hint, &list);

        Assert::IsTrue(e == NO_ERROR);
        Assert::IsNotNull(list);

        // sockaddr_in6 sin6{};
        addrinfo* iter = list;
        while (iter != nullptr)
        {
            uint16_t port = 0;
            // ipv4
            if (iter->ai_family == AF_INET)
            {
                sockaddr_in* v4a =
                    reinterpret_cast<sockaddr_in*>(iter->ai_addr);
                port = ntohs(v4a->sin_port);
            }
            else if (iter->ai_family == AF_INET6)
            {
                sockaddr_in6* v6a =
                    reinterpret_cast<sockaddr_in6*>(iter->ai_addr);
                port = ntohs(v6a->sin6_port);
            }
            iter = iter->ai_next;
        }

        ::freeaddrinfo(list);
    }

    TEST_METHOD(ResolveToAccept)
    {
        error_t e{};
        // char name[NI_MAXHOST]{};
        // char serv[NI_MAXSERV]{};
        const char* name = nullptr;
        const char* serv = "45678";

        // for `bind()`, `listen()`    // TCP + IPv6
        addrinfo hint =
            make_hint(AI_PASSIVE | AI_V4MAPPED, AF_INET6, SOCK_STREAM);
        addrinfo* list = nullptr;

        // Success : zero
        // Failure : non-zero uint32_t code
        e = ::getaddrinfo(
            name, serv, std::addressof(hint), std::addressof(list));

        Assert::IsTrue(e == NO_ERROR);
        Assert::IsNotNull(list);

        // sockaddr_in6 sin6{};
        addrinfo* iter = list;
        while (iter != nullptr)
        {
            uint16_t port = 0;
            // ipv4
            if (iter->ai_family == AF_INET)
            {
                sockaddr_in* v4a =
                    reinterpret_cast<sockaddr_in*>(iter->ai_addr);
                port = ntohs(v4a->sin_port);
            }
            else if (iter->ai_family == AF_INET6)
            {
                sockaddr_in6* v6a =
                    reinterpret_cast<sockaddr_in6*>(iter->ai_addr);
                port = ntohs(v6a->sin6_port);
            }

            iter = iter->ai_next;
        }

        ::freeaddrinfo(list);
    }

    TEST_METHOD(NameOfEndpoint)
    {
        error_t e{};
        sockaddr_in6 remote{};
        const char* name = "www.google.com";
        const char* serv = "https";

        // for `connect()`    // TCP + IPv6
        addrinfo hint = make_hint(AI_ALL | AI_V4MAPPED, AF_INET6, SOCK_STREAM);
        addrinfo* list = nullptr;

        // Success : zero
        // Failure : non-zero uint32_t code
        e = ::getaddrinfo(
            name, serv, std::addressof(hint), std::addressof(list));

        Assert::IsTrue(e == NO_ERROR);
        Assert::IsNotNull(list);

        // sockaddr_in6 sin6{};
        addrinfo* iter = list;
        while (iter != nullptr)
        {
            // continue
            if (iter->ai_family != AF_INET6) iter = iter->ai_next;

            sockaddr_in6* v6a = reinterpret_cast<sockaddr_in6*>(iter->ai_addr);
            remote = *v6a;
            break;
        }

        ::freeaddrinfo(list);

        // ensure `remote` is set...
        Assert::IsTrue(remote.sin6_port != 0);

        char name_buf[NI_MAXHOST]{};
        char serv_buf[NI_MAXSERV]{};

        const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&remote);
        // Success : zero
        // Failure : non-zero uint32_t code
        e = ::getnameinfo(ptr,
                          sizeof(sockaddr_in6),
                          name_buf,
                          NI_MAXHOST,
                          serv_buf,
                          NI_MAXSERV,
                          NI_NUMERICHOST | NI_NUMERICSERV);

        Assert::IsTrue(e == NO_ERROR);
    }

    TEST_METHOD(HostName)
    {
        error_t e{};
        char buf[NI_MAXHOST]{};

        e = ::gethostname(buf, NI_MAXHOST);
        Assert::IsTrue(e == NO_ERROR);
        Logger::WriteMessage(buf);
    }

    TEST_METHOD(SocketCreate)
    {
        error_t e{};
        SOCKET sd = ::WSASocketW(
            //AF_INET6, SOCK_RAW, IPPROTO_RAW, nullptr, 0, WSA_FLAG_OVERLAPPED);
            AF_INET6, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
        e = WSAGetLastError();

        Assert::IsTrue(e == NO_ERROR);
        Assert::IsTrue(sd != INVALID_SOCKET);

        closesocket(sd);
    }

    TEST_METHOD(SocketBind)
    {
        error_t e{};
        SOCKET sd = ::WSASocketW(
            //AF_INET6, SOCK_RAW, IPPROTO_RAW, nullptr, 0, WSA_FLAG_OVERLAPPED);
            AF_INET6, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

        e = WSAGetLastError();

        Assert::IsTrue(e == NO_ERROR);
        Assert::IsTrue(sd != INVALID_SOCKET);

        sockaddr_in6 local{};
        local.sin6_family = AF_INET6;
        local.sin6_addr = ::in6addr_any;
        local.sin6_port = ::htons(45677);

        e = bind(sd, reinterpret_cast<sockaddr*>(&local), sizeof(sockaddr_in6));
        Assert::IsTrue(e == NO_ERROR);

        closesocket(sd);
    }

    TEST_METHOD(SocketListen)
    {
        error_t e{};
        SOCKET sd = ::WSASocketW(AF_INET6,
                                 SOCK_STREAM,
                                 IPPROTO_TCP,
                                 nullptr,
                                 0,
                                 WSA_FLAG_OVERLAPPED);
        e = WSAGetLastError();

        Assert::IsTrue(e == NO_ERROR);
        Assert::IsTrue(sd != INVALID_SOCKET);

        sockaddr_in6 local{};
        local.sin6_family = AF_INET6;
        local.sin6_addr = ::in6addr_any;
        local.sin6_port = ::htons(45676);

        e = bind(sd, reinterpret_cast<sockaddr*>(&local), sizeof(sockaddr_in6));
        Assert::IsTrue(e == NO_ERROR);

        // auto backlog = SOMAXCONN;
        auto backlog = SOMAXCONN_HINT(7);
        e = listen(sd, backlog);
        Assert::IsTrue(e == NO_ERROR);

        closesocket(sd);
    }

    TEST_METHOD(SocketConnect)
    {
        error_t e{};
        // listener socket
        SOCKET ls = ::WSASocketW(AF_INET6,
                                 SOCK_STREAM,
                                 IPPROTO_TCP,
                                 nullptr,
                                 0,
                                 WSA_FLAG_OVERLAPPED);
        Assert::IsTrue(ls != INVALID_SOCKET);
        auto d1 = defer([=]() { closesocket(ls); });

        // client socket
        SOCKET cs = ::WSASocketW(AF_INET6,
                                 SOCK_STREAM,
                                 IPPROTO_TCP,
                                 nullptr,
                                 0,
                                 WSA_FLAG_OVERLAPPED);
        Assert::IsTrue(cs != INVALID_SOCKET);
        auto d2 = defer([=]() { closesocket(cs); });

        sockaddr_in6 local{};
        // address to listen
        local.sin6_family = AF_INET6;
        local.sin6_addr = ::in6addr_any;
        local.sin6_port = ::htons(45675);

        // prepare listener
        e = bind(ls, reinterpret_cast<sockaddr*>(&local), sizeof(sockaddr_in6));
        Assert::IsTrue(e == NO_ERROR);
        e = listen(ls, 1);
        Assert::IsTrue(e == NO_ERROR);

        // address to connect
        sockaddr_in6 remote{};
        remote.sin6_family = AF_INET6;
        remote.sin6_addr = ::in6addr_loopback;
        remote.sin6_port = ::htons(45675);

        e = connect(
            cs, reinterpret_cast<sockaddr*>(&remote), sizeof(sockaddr_in6));
        Assert::IsTrue(e == NO_ERROR);
    }
};
