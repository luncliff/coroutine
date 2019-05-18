//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

class coro_no_return_test : public test_adapter {
    // if it doesn't need to care about a coroutine's life cycle, use
    //  `no_return`. `co_return` with this type will destroy the frame
    static auto invoke_and_forget_frame() -> no_return {
        co_await suspend_never{};
        co_return;
    };

  public:
    void on_test() override {
        try {
            invoke_and_forget_frame();
        } catch (const exception& ex) {
            fail_with_message(ex.what());
        }
    };
};

class coro_frame_empty_test : public test_adapter {
  public:
    void on_test() override {
        frame fh{};
        auto coro = static_cast<coroutine_handle<void>>(fh);
        expect_true(coro.address() == nullptr);
    };
};

class coro_frame_return_test : public test_adapter {
    // when the coroutine frame destuction need to be controlled manually,
    //  just return `frame`. The type `frame` implements `return_void`,
    //  `suspend_never`(initial), `suspend_always`(final)
    static auto invoke_and_return() -> frame {
        co_await suspend_never{};
        co_return; // only void return
    };

  public:
    void on_test() override {
        auto frame = invoke_and_return();

        // allow access to `coroutine_handle<void>`
        //  after first suspend(which can be `co_return`)
        coroutine_handle<void>& coro = frame;
        expect_true(static_cast<bool>(coro)); // not null
        expect_true(coro.done());             // expect final suspended
        coro.destroy();
    };
};

class coro_frame_first_suspend_test : public test_adapter {
    static auto invoke_and_suspend() -> frame {
        co_await suspend_always{};
        co_return;
    };

  public:
    void on_test() override {
        auto frame = invoke_and_suspend();

        // allow access to `coroutine_handle<void>`
        //  after first suspend(which can be `co_return`)
        coroutine_handle<void>& coro = frame;
        expect_true(static_cast<bool>(coro)); // not null
        expect_true(coro.done() == false);    // it is susepended !
        coro.destroy();
    };
};

class coro_frame_awaitable_test : public test_adapter {
    static auto save_current_handle_to_frame(frame& fh, int& status)
        -> no_return {
        auto defer = gsl::finally([&]() {
            status = 3; // change state on destruction phase
        });
        status = 1;
        co_await fh; // frame holder is also an awaitable.
        status = 2;
        co_await fh;
        co_return;
    }

  public:
    void on_test() override {
        int status = 0;
        frame coro{};

        save_current_handle_to_frame(coro, status);
        expect_true(status == 1);

        // `frame` inherits `coroutine_handle<void>`
        coro.resume();
        expect_true(status == 2);

        // coroutine reached end.
        // so `defer` in the routine will change status
        coro.resume();
        expect_true(status == 3);
    };
};
