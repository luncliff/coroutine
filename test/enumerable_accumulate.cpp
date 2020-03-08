/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <numeric>

#include <coroutine/yield.hpp>

using namespace std;
using namespace coro;

auto yield_until_zero(int n) -> enumerable<int> {
    while (n-- > 0)
        co_yield n;
};

int main(int, char*[]) {
    auto g = yield_until_zero(10);
    auto total = accumulate(g.begin(), g.end(), 0u);
    assert(total == 45); // 0 - 10

    return EXIT_SUCCESS;
}
