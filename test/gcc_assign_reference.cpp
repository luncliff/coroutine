//
//  https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro.h
//
#include <cassert>
#include <string>

#include <coroutine/return.h>

using namespace std;
using namespace std::experimental;
using namespace coro;

auto assign_and_return(string& result) -> preserve_frame {
    // when co_return is commented, SegFault occurs
    co_await suspend_always{};
    result = "hello";
    co_return;
}

int main(int, char* []) {
    string result{};

    auto frame = assign_and_return(result);
    while (frame.done() == false)
        frame.resume();

    frame.destroy();
    puts(result.c_str());
    return 0;
}
