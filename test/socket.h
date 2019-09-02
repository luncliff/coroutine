//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
#include <coroutine/net.h>

void init_network_api() noexcept(false);
void release_network_api() noexcept;

//  create 1 socket
int64_t socket_create(const addrinfo& hint);

//  dispose given socket
void socket_close(int64_t sd);

//  bind the socket to given address
void socket_bind(int64_t sd, const sockaddr_in& local);
void socket_bind(int64_t sd, const sockaddr_in6& local);

//  start listen with the socket
void socket_listen(int64_t sd);

//  try connect to given endpoint
//  on failure, returns error code
int64_t socket_connect(int64_t sd, const sockaddr_in& remote);
int64_t socket_connect(int64_t sd, const sockaddr_in6& remote);

//  accept a connection request and return client socket
int64_t socket_accept(int64_t ln);

//  get the socket's address
//  on failure, returns error code
int64_t socket_get_name(int64_t sd, sockaddr_in& local);
int64_t socket_get_name(int64_t sd, sockaddr_in6& local);

//  get connected peer socket's address
//  on failure, returns error code
int64_t socket_get_peer(int64_t sd, sockaddr_in& remote);
int64_t socket_get_peer(int64_t sd, sockaddr_in6& remote);

//  change the socket's option
void socket_set_option(int64_t sd, int64_t level, int64_t option,
                       int64_t value);

//  make socket to operate in non-blocking mode
void socket_set_option_nonblock(int64_t sd);

//  make socket to reuse address
void socket_set_option_reuse_address(int64_t sd);

//  make tcp send without delay
void socket_set_option_nodelay(int64_t sd);

//  set the socket's send/recv timeout
void socket_set_option_timout(int64_t sd, uint32_t ms);

//  network related error. It's from `errno` or `WSAGetLastError`
int recent_net_error() noexcept;

//  test if the error code is because of non-blocking
bool is_in_progress(int ec) noexcept;
