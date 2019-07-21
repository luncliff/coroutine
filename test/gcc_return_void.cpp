//
//  https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro.h
//
#include <cstdio>

#include <coroutine/return.h>

using namespace std;
using namespace std::experimental;
using namespace coro;

auto return_void_and_destroy() -> no_return {
    co_await suspend_never{};
    co_return;
}

auto return_void_and_preserve(coroutine_handle<void>& coro) -> preserve_frame {
    co_await save_frame_to{coro};
    co_return;
}

#define REQUIRE(expr)                                                          \
    if ((expr) == false)                                                       \
        return __LINE__;

int main(int, char* []) {
    return_void_and_destroy();

    coroutine_handle<void> coro{};
    return_void_and_preserve(coro);

    REQUIRE(is_suspended(coro.prefix.g));
    REQUIRE(coro.done() == false);
    coro.resume();

    REQUIRE(coro.done());
    coro.destroy();
    return 0;
}
