//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <catch.hpp>

#include <numeric>
#include <utility>

#include <coroutine/frame.h>
#include <coroutine/unplug.hpp>

TEST_CASE("TypeSizeTest", "[layout]")
{
    struct frame_t
    {
        uint32_t a, b, c, d;
    };

    using frame_ext_t = attach_u64_type_size<frame_t>;

    using namespace std;
    frame_ext_t frame{};

    CAPTURE(addressof(frame));
    CAPTURE(addressof(frame.type_size));

    const auto s = sizeof(frame.type_size);
    REQUIRE(s == sizeof(uint64_t));

    auto d = distance((uint8_t*)addressof(frame),
                      (uint8_t*)addressof(frame.type_size));
    REQUIRE(d == sizeof(frame_t));

    REQUIRE(frame.type_size == sizeof(frame_t) + sizeof(uint64_t));

    auto* psize = reinterpret_cast<uint64_t*>(&frame + 1);
    REQUIRE(psize[-1] == frame.type_size);
}
