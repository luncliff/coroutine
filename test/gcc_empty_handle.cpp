//
//  https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro.h
//  https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro-builtins.C
//

#include <coroutine/frame.h>

using namespace std;
using namespace std::experimental;

int main(int, char* []) {
    auto coro = coroutine_handle<void>::from_address(nullptr);
    if (coro) {
        coro.done();
        coro.destroy();
    }
    return 0;
}
