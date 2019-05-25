//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

// if it doesn't need to care about a coroutine's life cycle, use
//  `no_return`. `co_return` with this type will destroy the frame
auto invoke_and_forget_frame() -> no_return {
    co_await suspend_never{};
    co_return;
};
auto coro_no_return_test() {
    try {
        invoke_and_forget_frame();
    } catch (const exception& ex) {
        FAIL_WITH_MESSAGE(string{ex.what()});
    }
};

auto coro_frame_empty_test() {
    frame fh{};
    auto coro = static_cast<coroutine_handle<void>>(fh);
    REQUIRE(coro.address() == nullptr);
};

// when the coroutine frame destuction need to be controlled manually,
//  just return `frame`. The type `frame` implements `return_void`,
//  `suspend_never`(initial), `suspend_always`(final)
auto invoke_and_return() -> frame {
    co_await suspend_never{};
    co_return; // only void return
};
auto coro_frame_return_test() {

    auto frame = invoke_and_return();

    // allow access to `coroutine_handle<void>`
    //  after first suspend(which can be `co_return`)
    coroutine_handle<void>& coro = frame;
    REQUIRE(static_cast<bool>(coro)); // not null
    REQUIRE(coro.done());             // expect final suspended
    coro.destroy();
};

auto invoke_and_suspend() -> frame {
    co_await suspend_always{};
    co_return;
};
auto coro_frame_first_suspend_test() {
    auto frame = invoke_and_suspend();

    // allow access to `coroutine_handle<void>`
    //  after first suspend(which can be `co_return`)
    coroutine_handle<void>& coro = frame;
    REQUIRE(static_cast<bool>(coro)); // not null
    REQUIRE(coro.done() == false);    // it is susepended !
    coro.destroy();
};

auto save_current_handle_to_frame(frame& fh, int& status) -> no_return {
    auto defer = gsl::finally([&]() {
        status = 3; // change state on destruction phase
    });
    status = 1;
    co_await fh; // frame holder is also an awaitable.
    status = 2;
    co_await fh;
    co_return;
}
auto coro_frame_awaitable_test() {
    int status = 0;
    frame coro{};

    save_current_handle_to_frame(coro, status);
    REQUIRE(status == 1);

    // `frame` inherits `coroutine_handle<void>`
    coro.resume();
    REQUIRE(status == 2);

    // coroutine reached end.
    // so `defer` in the routine will change status
    coro.resume();
    REQUIRE(status == 3);
};

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("no return from coroutine", "[return]") {
    coro_no_return_test();
}
TEST_CASE("empty frame object", "[return]") {
    coro_frame_empty_test();
}
TEST_CASE("frame after return", "[return]") {
    coro_frame_return_test();
}
TEST_CASE("frame after suspend", "[return]") {
    coro_frame_first_suspend_test();
}
TEST_CASE("frame supports co_await", "[return]") {
    coro_frame_awaitable_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_no_return : public TestClass<coro_no_return> {
    TEST_METHOD(test_coro_no_return) {
        coro_no_return_test();
    }
};
class coro_frame_empty : public TestClass<coro_frame_empty> {
    TEST_METHOD(test_coro_frame_empty) {
        coro_frame_empty_test();
    }
};
class coro_frame_return : public TestClass<coro_frame_return> {
    TEST_METHOD(test_coro_frame_return) {
        coro_frame_return_test();
    }
};
class coro_frame_first_suspend : public TestClass<coro_frame_first_suspend> {
    TEST_METHOD(test_coro_frame_first_suspend) {
        coro_frame_first_suspend_test();
    }
};
class coro_frame_awaitable : public TestClass<coro_frame_awaitable> {
    TEST_METHOD(test_coro_frame_awaitable) {
        coro_frame_awaitable_test();
    }
};

#endif