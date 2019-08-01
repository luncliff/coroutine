//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <gsl/gsl>

#include "test.h"
using namespace coro;

auto save_current_handle_to_frame(coroutine_handle<void>& frame, //
                                  int& status) -> preserve_frame {
    auto on_return = gsl::finally([&status]() {
        // change the status when the frame is going to be destroyed
        status += 1;
    });

    status += 1;
    co_await save_frame_t{frame};
    status += 1;
}

auto coro_preserve_frame_destroy_with_handle() {
    int status = 0;
    coroutine_handle<void> coro{};

    {
        save_current_handle_to_frame(coro, status);
        _require_(status == 1);
        _require_(coro.address() != nullptr);
    }

    coro.resume();

    // the coroutine reached end
    // so, gsl::finally should changed the status
    _require_(coro.done());
    _require_(status == 3);

    coro.destroy();

    return EXIT_SUCCESS;
}

#
#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return coro_preserve_frame_destroy_with_handle();
}

#elif __has_include(<CppUnitTest.h>)
class coro_forget_frame : public TestClass<coro_forget_frame> {
    TEST_METHOD(test_coro_forget_frame) {
        coro_forget_frame_test();
    }
};
#endif
