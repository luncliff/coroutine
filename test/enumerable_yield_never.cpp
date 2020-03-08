/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>

#include <coroutine/yield.hpp>

using namespace std;
using namespace coro;

auto yield_never() -> enumerable<int> {
    // co_return is necessary so compiler can notice that
    // this is a coroutine when there is no co_yield.
    co_return;
};

int main(int, char*[]) {
    auto count = 0u;
    for (auto v : yield_never()) {
        v = count; // code to suppress C4189
        count += 1;
    }
    assert(count == 0);
    return EXIT_SUCCESS;
}
