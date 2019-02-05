// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <coroutine/return.h>
#include <coroutine/sync.h>
#include <gsl/gsl>

#include <CppUnitTest.h>
#include <sdkddkver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace gsl;

void create_bound_socket(SOCKET& sd, sockaddr_in6& ep, const addrinfo& hint,
                         gsl::czstring<> host, gsl::czstring<> port)
{
    // create socket
    sd = ::WSASocketW(hint.ai_family, hint.ai_socktype, hint.ai_protocol,
                      nullptr, 0, WSA_FLAG_OVERLAPPED);
    Assert::IsTrue(sd != INVALID_SOCKET);

    // acquire address
    for (auto e : resolve(hint, host, port))
        ep = e;
    Assert::IsTrue(ep.sin6_port != 0);

    // bind socket and address
    auto ec = ::bind(sd, reinterpret_cast<sockaddr*>(&ep), sizeof(ep));
    Assert::IsTrue(ec == NO_ERROR);
}
