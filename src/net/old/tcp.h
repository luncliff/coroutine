#include "../api.h"

namespace net
{
namespace tcp
{
// - Note
//      little endian format
struct little
{
    u8 ns : 1;     // ECN-Nounce
    u8 res : 3;    // Reserved
    u8 offset : 4; // Data offset
                   //
    u8 fin : 1;    // Finish
    u8 syn : 1;    // Sync Packet
    u8 rst : 1;    // Connection Reset
    u8 psh : 1;    // Push
    u8 ack : 1;    // Acknowledgment
    u8 urg : 1;    // Urgent Pointer
    u8 ece : 1;    // ECN-Echo
    u8 cwr : 1;    // Congestion Window Reduced
};
static_assert(sizeof(tcp::little) == 2, WRONG_TYPE_SIZE);

// - Note
//      big endian format
struct big
{
    u8 offset : 4; // Data offset
    u8 res : 3;    // Reserved
    u8 ns : 1;     // ECN-Nounce
                   //
    u8 cwr : 1;    // Congestion Window Reduced
    u8 ece : 1;    // ECN-Echo
    u8 urg : 1;    // Urgent Pointer
    u8 ack : 1;    // Acknowledgment
    u8 psh : 1;    // Push
    u8 rst : 1;    // Connection Reset
    u8 syn : 1;    // Sync Packet
    u8 fin : 1;    // Finish
};
static_assert(sizeof(tcp::big) == 2, WRONG_TYPE_SIZE);

// - Note
//      TCP fixed header
// - Caution
//      Assuming Network byte order (Big endian)
struct header
{
    u16 src;     // Source port
    u16 dst;     // Destination port
    u32 seq_num; // Sequence packet number
    u32 ack_num; // Acked sequence packet number

    union {
        little le; // little endian format
        big be;    // big endian format
    };

    u16 window;   // Available window size
    u16 checksum; // TCP checksum
    u16 urgent;   // Urgent pointer
};
static_assert(sizeof(tcp::header) == 20, WRONG_TYPE_SIZE);

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//  - IPPROTO_TCP level :
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms738596(v=vs.85).aspx
//
//     g  s   Option Value                Description
//    ------  --------------------------- ---------------------------------
//    [ ][ ]  TCP_OFFLOAD_NO_PREFERENCE   //
//    [ ][ ]  TCP_OFFLOAD_NOT_PREFERRED   //
//    [ ][ ]  TCP_OFFLOAD_PREFERRED       //
//    [ ][ ]  TCP_NODELAY                 //+ defined in <ws2def.h>
//    [ ][ ]  TCP_EXPEDITED_1122          //+
//    [ ][ ]  TCP_KEEPALIVE               //
//    [ ][ ]  TCP_MAXSEG                  //
//    [ ][ ]  TCP_MAXRT                   //+
//    [ ][ ]  TCP_STDURG                  //
//    [ ][ ]  TCP_NOURG                   //
//    [ ][ ]  TCP_ATMARK                  //
//    [ ][ ]  TCP_NOSYNRETRIES            //
//    [ ][ ]  TCP_TIMESTAMPS              //+
//    [ ][ ]  TCP_OFFLOAD_PREFERENCE      //
//    [ ][ ]  TCP_CONGESTION_ALGORITHM    //
//    [ ][ ]  TCP_DELAY_FIN_ACK           //
//    [ ][ ]  TCP_MAXRTMS                 //
//    [ ][ ]  TCP_FASTOPEN                //+
//

// ...
} // tcp
} // net