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

auto sequence_suspend_with_await(coroutine_handle<void>& rh)
    -> sequence<status_t> {
    status_t value = 0;
    co_yield value = 1;
    co_await save_frame_t{rh}; // use `co_await` instead of `co_yield`
    co_yield value = 2;
}
auto use_sequence_yield_suspend_yield_1(status_t& ref,
                                        coroutine_handle<void>& rh)
    -> preserve_frame {
    // clang-format off
    for co_await(auto v : sequence_suspend_with_await(rh))
        ref = v;
    // clang-format on
};

auto coro_sequence_suspend_using_await_test() {
    preserve_frame fs{};
    status_t storage = -1;
    auto fc = use_sequence_yield_suspend_yield_1(storage, fs);
    auto on_return = gsl::finally([&fc]() {
        if (fc)
            fc.destroy();
    });

    _require_(fs.done() == false); // we didn't finished iteration
    _require_(storage == 1);       // co_yield value = 1

    // continue after co_await manual_resume;
    fs.resume();
    _require_(storage == 2); // co_yield value = 2;
    _require_(fs.done() == true);
    _require_(fc.done() == true);

    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_sequence_suspend_using_await_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_sequence_suspend_using_await
    : public TestClass<coro_sequence_suspend_using_await> {
    TEST_METHOD(test_coro_sequence_suspend_using_await) {
        coro_sequence_suspend_using_await_test();
    }
};
#endif
