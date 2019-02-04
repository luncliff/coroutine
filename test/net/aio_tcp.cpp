//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>
#include <coroutine/return.h>

#include <thread>

#include <fcntl.h>
#include <unistd.h>

using packet_chunk_t = std::array<gsl::byte, 2842>;
using namespace std::chrono_literals;

extern int make_async_socket(int family, int type, int proto) noexcept(false);
extern void dispose_socket(uint64_t sd);

auto coro_recv_stream
    = [](int sd, packet_chunk_t& buffer, uint32_t flag) -> unplug {
    io_work_t wk{};
    int64_t rsz = co_await recv_stream(sd, buffer, flag, wk);
    if (rsz < 0)
        FAIL(strerror(errno));

    REQUIRE(static_cast<size_t>(rsz) <= buffer.size());
};

auto coro_send_stream
    = [](int sd, packet_chunk_t& buffer, uint32_t flag) -> unplug {
    io_work_t wk{};
    int64_t ssz = co_await send_stream(sd, buffer, flag, wk);
    if (ssz < 0)
        FAIL(strerror(errno));

    REQUIRE(static_cast<size_t>(ssz) <= buffer.size());
};
