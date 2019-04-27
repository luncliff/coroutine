//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
#include <coroutine/net.h>

void load_network_api() noexcept(false);

//  create 1 socket
int64_t socket_create(const addrinfo& hint);

//  create multiple socket
auto socket_create(const addrinfo& hint, size_t count)
    -> coro::enumerable<int64_t>;

//  dispose given socket
void socket_close(int64_t sd);

//  bind the socket to given address
void socket_bind(int64_t sd, endpoint_t& ep);

//  start listen with the socket
void socket_listen(int64_t sd);

//  make socket to operate in non-blocking mode
void socket_set_option_nonblock(int64_t sd);

//  make socket to reuse address
void socket_set_option_reuse_address(int64_t sd);

//  make tcp send without delay
void socket_set_option_nodelay(int64_t sd);
