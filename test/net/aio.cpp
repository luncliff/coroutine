//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch2/catch.hpp>

#include <coroutine/net.h>
#include <coroutine/return.h>

#include <future>

using packet_chunk_t = std::array<gsl::byte, 2842>;

auto apply_nonblock_async(int sd)
{
    REQUIRE(fcntl(sd, F_SETFL, // non block
                               //  allow sigio redirection
                  O_NONBLOCK | O_ASYNC | fcntl(sd, F_GETFL, 0))
            != -1);
};

auto coro_recv_from(int sd,              //
                    sockaddr_in& remote, //
                    packet_chunk_t& buffer) -> unplug
{
    io_work_t wk{};
    auto rsz = co_await recv_from(sd, remote, buffer, wk);

    REQUIRE(rsz == buffer.size());
};

auto coro_send_to(int sd,                    //
                  const sockaddr_in& remote, //
                  packet_chunk_t& buffer) -> unplug
{
    io_work_t wk{};
    auto ssz = co_await send_to(sd, remote, buffer, wk);
    REQUIRE(ssz == buffer.size());
};

SCENARIO("async network api", "[network]")
{

    GIVEN("udp ipv4 socket")
    {
        // service address for bind
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port = htons(11320);
        local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        // use service in loopback address
        sockaddr_in remote{};
        remote.sin_family = AF_INET;
        remote.sin_port = htons(11320);
        remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        auto rs = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        REQUIRE(rs > 0);
        auto d1 = gsl::finally([=]() noexcept { close(rs); });
        apply_nonblock_async(rs);

        auto ws = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        REQUIRE(ws > 0);
        auto d2 = gsl::finally([=]() noexcept { close(ws); });
        apply_nonblock_async(ws);

        WHEN("recv and then send")
        {
            auto chunk = std::make_unique<packet_chunk_t>();

            if (bind(rs, (sockaddr*)&local, sizeof(sockaddr_in)) == -1)
                FAIL(strerror(errno));

            coro_recv_from(rs, local, *chunk);
            coro_send_to(ws, remote, *chunk);

            // run on another thread
            auto f = std::async(std::launch::async, []() {
                auto count = 0;
                while (count < 2) // the library recommends
                                  // to continue loop without break
                                  // so that there is no leak of event
                    for (auto task : fetch_io_tasks())
                    {
                        task.resume();
                        ++count;
                    }
                REQUIRE(count == 2);
            });
            REQUIRE_NOTHROW(f.get());
        }
    }
}
