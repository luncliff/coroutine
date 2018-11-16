// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <array>
#include <functional>
#include <numeric>

static auto defer(std::function<void(void)> fn)
{
    struct defer_t
    {
        std::function<void(void)> f{};

        defer_t(std::function<void(void)> _f) : f{_f} {}
        ~defer_t() { f(); }
    };
    return defer_t{fn};
}

#endif // TEST_HELPER_H
