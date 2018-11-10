// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "../linkable.h"

#include <coroutine/frame.h>

// We are using VC++ headers, but the compiler is not msvc.
// Redirect some intrinsics from msvc to clang
#ifndef __clang__
#error "This file must be compiled with clang 6.0 or later";
#else

void dump_clang_frame(clang_frame* _frame) noexcept
{
    static constexpr auto frame_length = 14;
    auto& view = *reinterpret_cast<std::array<uint64_t, frame_length>*>(_frame);

    std::printf("frame address: %p %p \n", view.data(), _frame);
    for (auto offset = 0u; offset < view.size(); ++offset)
    {
        std::printf(
            "b: %02u %p %016llx\n", offset, view.data() + offset, view[offset]);
    }
    std::puts("\n");
}

#endif
