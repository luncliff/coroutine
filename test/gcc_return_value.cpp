//
//  https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro.h
//
#include <string>

#include <coroutine/return.h>

using namespace std;
using namespace std::experimental;
using namespace coro;

#define REQUIRE(expr)                                                          \
    if ((expr) == false)                                                       \
        return __LINE__;

auto spawn_time_consuming_task(int ev, string& result) -> no_return {
    // when co_return is commented, SegFault occurs
    co_return;
}
auto wait_on_event(int ev) -> void {
}

auto wait_using_await(int ev) -> no_return {
    co_await suspend_never{};
}

int main(int, char* []) {
    string result{};
    int ev{};
    spawn_time_consuming_task(ev, result);
    wait_on_event(ev);

    return 0;
}
