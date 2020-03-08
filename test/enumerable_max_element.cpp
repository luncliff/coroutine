/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <array>
#include <cassert>

#include <coroutine/yield.hpp>

using namespace std;
using namespace coro;

auto yield_with_container(array<uint32_t, 10>& storage)
    -> enumerable<uint32_t> {
    for (auto& ref : storage)
        co_yield ref;
};

int main(int, char*[]) {
    // since generator is not a container,
    //  using max_element (or min_element) function on it
    //  will return invalid iterator
    auto id = 15u;
    array<uint32_t, 10> storage{};
    for (auto& e : storage)
        e = id--; // [15, 14, 13 ...
                  // so the first element will hold the largest number

    auto g = yield_with_container(storage);
    auto it = max_element(g.begin(), g.end());

    // after iteration is finished (`co_return`),
    // the iterator will hold nullptr.
    assert(it.operator->() == nullptr);

    // so referencing it will lead to access violation
    // _require_(*it == 15);
    return EXIT_SUCCESS;
}
