// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <coroutine/return.h>
#include <gsl/gsl>

#include <CppUnitTest.h>
#include <sdkddkver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using gsl::byte;
using gsl::finally;

class socket_udp_multicast_test : public TestClass<socket_udp_multicast_test>
{
    TEST_METHOD_INITIALIZE(setup)
    {
        // create 3 socket. A, B, C

        // A register (+ loopback)
        // B register (- loopback)

        // start a service
    }

    TEST_METHOD_CLEANUP(teardown)
    {
        // stop the service
    }

    TEST_METHOD(socket_udp_multicast)
    {
        // B send packet

        // A recv packet
        // B didn't
        // C didn't
    }

    //TEST_METHOD(socket_udp_multicast_with_loopback)
    //{
    //    // A send packet
    //    // B recv packet
    //    // A recv packet
    //    // C didn't
    //}
};
