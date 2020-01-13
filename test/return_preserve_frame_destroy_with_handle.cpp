//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cassert>

#include <coroutine/return.h>
#include <gsl/gsl>

using namespace std;
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

auto coro_preserve_frame_destroy_with_handle_test() {
    int status = 0;
    coroutine_handle<void> coro{};

    {
        save_current_handle_to_frame(coro, status);
        assert(status == 1);
        assert(coro.address() != nullptr);
    }

    coro.resume();

    // the coroutine reached end
    // so, gsl::finally should changed the status
    assert(coro.done());
    assert(status == 3);

    coro.destroy();

    return EXIT_SUCCESS;
}

#
#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_preserve_frame_destroy_with_handle_test();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_preserve_frame_destroy_with_handle
    : public TestClass<coro_preserve_frame_destroy_with_handle> {
    TEST_METHOD(test_coro_preserve_frame_destroy_with_handle) {
        coro_preserve_frame_destroy_with_handle_test();
    }
};
#endif
