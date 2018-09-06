//// ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ====
////
////  Author   : Park Dong Ha ( luncliff@gmail.com )
////
////  Note
////      Proactor pattern support
////
////  See Also
////      Boost ASIO
////
//// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//#ifndef _NET_IO_SERVICE_H_
//#define _NET_IO_SERVICE_H_
//
//#include "./socket.h"
//#include "./protocol/tcp.h"
//
//namespace net
//{
//class service
//{
//  iocp cp;
//
//public:
//  explicit service(uint16_t max = 1) noexcept;
//  ~service() noexcept = default;
//
//public:
//  error poll(uint32_t timeout) noexcept;
//  error bind(SOCKET sd) noexcept;
//
//  SOCKET open(category socktype, proto ipproto) noexcept;
//  SOCKET dial(const endpoint &remote) noexcept;
//  SOCKET listen(const endpoint &local,
//                int backlog = 24) noexcept;
//};
//static_assert(sizeof(iocp) == sizeof(service),
//              WRONG_TYPE_SIZE);
//
//} // net
//#endif
