/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>

#include <coroutine/yield.hpp>

using namespace std;
using namespace coro;

auto yield_once(int value = 0) -> enumerable<int> {
    co_yield value;
    co_return;
};

int main(int, char*[]) {
    auto count = 0u;
    for (auto v : yield_once()) {
        assert(v == 0);
        count += 1;
    }
    assert(count > 0);
    return EXIT_SUCCESS;
}
