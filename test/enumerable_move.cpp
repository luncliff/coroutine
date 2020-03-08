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
    auto m = move(g);
    // g lost its handle. so it is not iterable anymore
    assert(g.begin() == g.end());
    for (auto v : g) {
        v = count;       // code to suppress C4189
        return __LINE__; // null generator won't go into loop
    }

    for (auto v : m) {
        assert(v == 0);
        count += 1;
    }
    assert(count > 0);
    return EXIT_SUCCESS;
}
