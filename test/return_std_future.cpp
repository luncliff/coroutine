/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <gsl/gsl>

#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto redirect_return_to_future(int lhs, int rhs) -> std::future<int> {
    co_await suspend_never{};
    co_return lhs + rhs;
}

int main(int, char*[]) {
    auto f = redirect_return_to_future(1, 2);
    assert(f.get() > 0);
    return 0;
}
