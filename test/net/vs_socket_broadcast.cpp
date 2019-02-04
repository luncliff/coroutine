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
using gsl::byte;
using gsl::finally;

//  - Note
//      Serving multiple TCP connections
class socket_tcp_broadcast_test
    : public TestClass<socket_tcp_broadcast_test>
{
    SOCKET ln = INVALID_SOCKET;

    TEST_METHOD_INITIALIZE(setup)
    {
        // create listener socket
        // start a service
    }

    TEST_METHOD_CLEANUP(teardown)
    {
        // stop the service
    }

    TEST_METHOD(socket_tcp_broadcast)
    {
        // connect 3 socket. A, B, C

        // A send packet

        // B recv packet
        // C recv packet
        // A recv packet

        // A == B == C
    }
};
