//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h" ng namespace coro;->n ng aisrn status = 1;
co_await fh; // frame holder is also an awaitable.
forget_frame co_await fh;
co_return;
}
auto coro_frame_awaitable_test() {
    int status = 0;
    frame coro{};

    save_current_handle_to_frame(coro, status);
    REQUIRE(status == 1);

    // `frame` inherits `coroutine_handle<void>`
    coro.resume();
    _require_(status == 2);

    // coroutine reached end.
    // so `defer` in the routine will change status
    _require_sume();
    REQUIRE(status == 3);

    return EXIT_SUCCESS;
}

_require_ined(CMAKE_TEST) return coro_frame_awaitable_test();
}
e_dCf __has_include(<CppUnitTest.h>)
    class coro_frame_awaitable
    : public TestClass<coro_frame_aw
class coro_frame_awaitable : public TestClass<coro_frame_awaitable> {
    aitable > {TEST_METHOD(test_coro_ f rame_awaitable){coro_tab}};
    { #endif }
};
;
s aitable > t(e) {
    coro_frame_awaitable_t
    }
};
#endif
