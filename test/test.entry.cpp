//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <coroutine/frame.h>

void dump_frame(void* frame) noexcept
{
    static constexpr auto frame_length = 20;
    auto& view = *reinterpret_cast<std::array<uint64_t, frame_length>*>(frame);

    std::printf("dump_frame: %p \n", frame);
    for (auto offset = 0u; offset < view.size(); ++offset)
    {
        std::printf(
            "%02u %p %016llx\n", offset, view.data() + offset, view[offset]);
    }
    std::puts("\n");
}
