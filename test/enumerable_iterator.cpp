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
    auto g = yield_once();
    auto b = g.begin();
    auto e = g.end();
    for (auto it = b; it != e; ++it) {
        assert(*it == 0);
        count += 1;
    }
    assert(count > 0);
    return EXIT_SUCCESS;
}
