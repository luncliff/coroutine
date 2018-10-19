// ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ====
//
//  Author  : Park Dong Ha (luncliff@gmail.com)
//
//  Note
//      Additional UDP implementations
//
//  To Do
//      More implementation + Test
//
//  Reference
//      -
//
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#ifndef _NET_UDPV6_H_
#define _NET_UDPV6_H_

#include <string>
#include <cstring>
#include "../api.h"

namespace net
{
namespace udp
{

// - Note
//      UDP Header
// - Caution
//      All values must be stored as big endian manually.
//      Checksum is mandatory for IPv6
// - Reference
//      https://en.wikipedia.org/wiki/User_Datagram_Protocol
struct header
{
    u16 src;
    u16 dst;
    u16 length;
    u16 checksum;
};
static_assert(sizeof(header) == 8, WRONG_TYPE_SIZE);

//
//  - IPPROTO_UDP level :
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms738603(v=vs.85).aspx
//

// ...
} // udp
} // net

#endif
