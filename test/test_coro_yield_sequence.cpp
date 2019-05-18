//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

class coro_sequence_no_yield_test : public test_adapter {
    static auto sequence_yield_never() -> sequence<int> {
        co_return;
    }

  public:
    void on_test() override {
        auto use_async_gen = [](int& ref) -> frame {
            // clang-format off
            for co_await(auto v : sequence_yield_never())
                ref = v;
            // clang-format on
        };

        frame f1{}; // frame holder
        auto no_frame_leak = gsl::finally([&]() {
            if (f1.address() != nullptr)
                f1.destroy();
        });
        int storage = -1;
        // since there was no yield, it will remain unchanged
        f1 = use_async_gen(storage);
        expect_true(storage == -1);
    }
};

class coro_sequence_frame_status_test : public test_adapter {
    static auto sequence_suspend_and_return(frame& fh) -> sequence<int> {
        co_yield fh; // save current coroutine to the `frame`
                     // the `frame` type inherits `suspend_always`
        co_return;   // return without yield
    }

  public:
    void on_test() override {
        auto use_async_gen = [](int& ref, frame& fs) -> frame {
            // clang-format off
            for co_await(auto v : sequence_suspend_and_return(fs))
                ref = v;
            // clang-format on
            ref = 2; // change the value after iteration
        };

        frame fc{}, fs{}; // frame of caller, frame of sequence
        auto no_frame_leak = gsl::finally([&]() {
            // notice that `sequence<int>` is in the caller's frame,
            //  therefore, by destroying caller, sequence<int> will be destroyed
            //  automatically. so accessing to the frame will lead to violation
            // just destroy caller coroutine's frame
            fc.destroy();
        });
        int storage = -1;
        // since there was no yield, it will remain unchanged
        fc = use_async_gen(storage, fs);
        expect_true(storage == -1);
        // however, when the sequence coroutine is resumed,
        // its caller will continue on ...
        expect_true(fs.address() != nullptr);
        fs.resume(); // sequence coroutine will be resumed
                     //  and it will resume `use_async_gen`.
        expect_true(
            fs.done()); //  since `use_async_gen` will return after that,
        expect_true(
            fc.done()); //  both coroutine will reach *final suspended* state
        expect_true(storage == 2);
    }
};

class coro_sequence_yield_once_test : public test_adapter {
    static auto sequence_yield_once(int value = 0) -> sequence<int> {
        co_yield value;
        co_return;
    }

  public:
    void on_test() override {
        auto use_async_gen = [](int& ref) -> frame {
            // clang-format off
            for co_await(auto v : sequence_yield_once(0xBEE))
                ref = v;
            // clang-format on
        };

        frame fc{}; // frame of caller
        auto no_frame_leak = gsl::finally([&]() {
            if (fc.address() != nullptr)
                fc.destroy();
        });
        int storage = -1;
        fc = use_async_gen(storage);
        expect_true(storage == 0xBEE); // storage will receive value
    }
};

class coro_sequence_suspend_using_await_test : public test_adapter {
    static auto sequence_yield_suspend_yield_1(frame& manual_resume)
        -> sequence<int> {
        auto value = 0;
        co_yield value = 1;
        co_await manual_resume;
        co_yield value = 2;
    }

    frame fc{}, fs{}; // frame of caller, frame of sequence

  public:
    void on_test() override {
        auto no_frame_leak = gsl::finally([&]() {
            // see `sequence_suspend_and_return`'s test case
            if (fc.address() != nullptr)
                fc.destroy();
        });
        int storage = -1;
        auto use_async_gen = [](int& ref, frame& fh) -> frame {
            // clang-format off
            for co_await(auto v : sequence_yield_suspend_yield_1(fh))
                ref = v;
            // clang-format on
        };
        fc = use_async_gen(storage, fs);
        expect_true(fc.done() == false); // we didn't finished iteration
        expect_true(fs.done() == false); // co_await manual_resume
        expect_true(storage == 1);       // co_yield value = 1
        fs.resume();               // continue after co_await manual_resume;
        expect_true(storage == 2); // co_yield value = 2;
        expect_true(fc.done());    // both are finished
        expect_true(fs.done());
    }
};

class coro_sequence_suspend_using_yield_test : public test_adapter {
    static auto sequence_yield_suspend_yield_2(frame& manual_resume)
        -> sequence<int> {
        auto value = 0;
        co_yield value = 1;
        co_yield manual_resume; // use `co_yield` instead of `co_await`
        co_yield value = 2;
    };
    frame fc{}, fs{}; // frame of caller, frame of sequence

  public:
    void on_test() override {
        auto no_frame_leak = gsl::finally([&]() {
            // see `sequence_suspend_and_return`'s test case
            if (fc.address() != nullptr)
                fc.destroy();
        });
        int storage = -1;
        auto use_async_gen = [](int& ref, frame& fh) -> frame {
            // clang-format off
            for co_await(auto v : sequence_yield_suspend_yield_2(fh))
                ref = v;
            // clang-format on
        };
        fc = use_async_gen(storage, fs);
        expect_true(fc.done() == false); // we didn't finished iteration
        expect_true(fs.done() == false); // co_await manual_resume
        expect_true(storage == 1);       // co_yield value = 1
        fs.resume();               // continue after co_await manual_resume;
        expect_true(storage == 2); // co_yield value = 2;
        expect_true(fc.done());    // both are finished
        expect_true(fs.done());
    }
};

class coro_sequence_destroy_when_suspended_test : public test_adapter {
    static auto sequence_yield_suspend_yield_1(frame& manual_resume)
        -> sequence<int> {
        auto value = 0;
        co_yield value = 1;
        co_await manual_resume;
        co_yield value = 2;
    }

  public:
    void on_test() override {
        auto use_async_gen = [](int* ptr, frame& fh) -> frame {
            auto defer = gsl::finally([=]() {
                *ptr = 0xDEAD; // set the value in destruction phase
            });
            // clang-format off
        for co_await(auto v : sequence_yield_suspend_yield_1(fh))
            *ptr = v;
        // clang-format on
        };

        int storage = -1;
        frame fs{}; // frame of sequence
        auto fc = use_async_gen(&storage, fs);

        expect_true(fs.done() == false); // it is suspended. of course false !
        expect_true(storage == 1);       // co_yield value = 1
        fc.destroy();
        // as mentioned in `sequence_suspend_and_return`, destruction of `fs`
        //  will lead access violation in caller frame destruction.
        // however, it is still safe to destroy caller frame
        //  and it won't be a resource leak
        //  since the step also destroys frame of sequence
        expect_true(storage == 0xDEAD); // see gsl::finally
    }
};
