//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>
#include <cstdio>
#include <exception>

#include <coroutine/return.h>

using namespace std;
using namespace coro;

auto invoke_and_forget_frame() -> frame_t {
    co_await suspend_never{};
    co_return;
};

int main(int, char* []) try {
    invoke_and_forget_frame();
    return EXIT_SUCCESS;

} catch (const exception& ex) {
    fputs(ex.what(), stderr);
    return __LINE__;
}
