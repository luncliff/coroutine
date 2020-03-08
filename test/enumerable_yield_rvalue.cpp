/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <string>

#include <coroutine/yield.hpp>

using namespace std;
using namespace coro;

auto yield_rvalue() -> enumerable<string> {
    co_yield "1";
    co_yield "2";
    co_yield "3";
};

int main(int, char*[]) {
    string txt{};
    for (auto v : yield_rvalue()) {
        txt += v;
        txt += ",";
    }
    assert(txt == "1,2,3,");
    return EXIT_SUCCESS;
}
