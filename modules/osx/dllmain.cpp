// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------

#define PROCEDURE __attribute__((constructor))

#include <cassert>
#include <numeric>

#include <coroutine/sequence.hpp>
#include <coroutine/unplug.hpp>

namespace coroutine
{
auto check_coroutine_available() noexcept -> unplug
{
    co_await std::experimental::suspend_never{};
}

auto check_generator_available() noexcept
    -> std::experimental::generator<uint16_t>
{
    uint16_t value = 4;
    co_yield value;
    co_yield value;
    co_return;
}

// this function is reserved for library initialization
// for now, it just checks some coroutine expressions
PROCEDURE void on_load_test(void*) noexcept(false)
{
    check_coroutine_available();
    auto g = check_generator_available();
    auto sum = std::accumulate(g.begin(), g.end(), 0);
    assert(sum == 4 * 2);
}

} // namespace coroutine
