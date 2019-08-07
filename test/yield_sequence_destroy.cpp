//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/sequence.hpp>

#include "test.h"
using namespace std;
using namespace coro;

using status_t = int64_t;

auto sequence_suspend_with_yield(coroutine_handle<void>& rh)
    -> sequence<status_t>;

auto use_sequence_yield_suspend_yield_final(status_t* ptr,
                                            coroutine_handle<void>& rh)
    -> preserve_frame {
    auto on_return = gsl::finally([=]() {
        *ptr = 0xDEAD; // set the value in destruction phase
    });
    // clang-format off
    for co_await(auto v : sequence_suspend_with_yield(rh))
        *ptr = v;
    // clang-format on
};

auto coro_sequence_destroy_when_suspended_test() {
    status_t storage = -1;
    coroutine_handle<void> fs{}; // frame of sequence

    auto fc = use_sequence_yield_suspend_yield_final(&storage, fs);
    _require_(fs.done() == false); // it is suspended. of course false
    _require_(storage == 1);       // co_yield value = 1

    // manual destruction of the coroutine frame
    fc.destroy();
    // as mentioned in `sequence_suspend_and_return`,
    //  destruction of `fs` will lead access violation
    //  when its caller is resumed.
    // however, it is still safe to destroy caller frame and
    //  it won't be a resource leak
    //  since the step also destroys frame of sequence
    _require_(storage == 0xDEAD); // notice the gsl::finally

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_sequence_destroy_when_suspended_test();
}

auto sequence_suspend_with_yield(coroutine_handle<void>& rh)
    -> sequence<status_t> {
    status_t value = 0;
    co_yield value = 1;
    co_yield save_frame_t{rh}; // use `co_yield` instead of `co_await`
    co_yield value = 2;
};

#elif __has_include(<CppUnitTest.h>)
class coro_sequence_destroy_when_suspended
    : public TestClass<coro_sequence_destroy_when_suspended> {
    TEST_METHOD(test_coro_sequence_destroy_when_suspended) {
        coro_sequence_destroy_when_suspended_test();
    }
};

#endif
