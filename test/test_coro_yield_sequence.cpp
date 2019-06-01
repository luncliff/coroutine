//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;
using status_t = int64_t;

auto sequence_yield_never() -> sequence<status_t> {
    co_return;
}
auto use_sequence_yield_never(status_t& ref) -> frame {
    // clang-format off
    for co_await(auto v : sequence_yield_never())
        ref = v;
    // clang-format on
};

auto coro_sequence_no_yield_test() {
    frame f1{}; // frame holder for the caller
    auto on_return = gsl::finally([&f1]() {
        if (f1.address() != nullptr)
            f1.destroy();
    });

    status_t storage = -1;
    // since there was no yield, it will remain unchanged
    f1 = use_sequence_yield_never(storage);
    REQUIRE(storage == -1);
}

auto sequence_suspend_and_return(frame& fh) -> sequence<status_t> {
    co_yield fh; // save current coroutine to the `frame`
                 // the `frame` type inherits `suspend_always`
    co_return;   // return without yield
}
auto use_sequence_suspend_and_return(status_t& ref, frame& fs) -> frame {
    // clang-format off
    for co_await(auto v : sequence_suspend_and_return(fs))
        ref = v;
    // clang-format on
    ref = 2; // change the value after iteration
};

auto coro_sequence_frame_status_test() {
    // notice that `sequence<status_t>` is placed in the caller's frame,
    //  therefore, when the caller is returned, sequence<status_t> will be
    // destroyed in caller coroutine frame's destruction phase
    frame fs{};
    //  automatically. so using `destroy` to the frame manually will lead to
    //  access violation ...

    status_t status = -1;
    // since there was no yield, it will remain unchanged
    auto fc = use_sequence_suspend_and_return(status, fs);
    REQUIRE(status == -1);

    // however, when the sequence coroutine is resumed,
    // its caller will continue on ...
    REQUIRE(fs.address() != nullptr);

    // clang_frame_prefix* ptr = (clang_frame_prefix*)fc.address();
    // printf("%p %p \n", ptr->factivate, ptr->fdestroy);
    // for (auto i = 0u; i < 20; ++i) {
    //     auto* u = (uint64_t*)ptr;
    //     printf("%p\t%16lx \n", u + i, *(u + i));
    // }

    // ptr = (clang_frame_prefix*)fs.address();
    // printf("%p %p \n", ptr->factivate, ptr->fdestroy);
    // for (auto i = 0u; i < 20; ++i) {
    //     auto* u = (uint64_t*)ptr;
    //     printf("%p\t%16lx \n", u + i, *(u + i));
    // }

    // sequence coroutine will be resumed
    //  and it will resume `use_sequence_suspend_and_return`.
    fs.resume();
    //  since `use_sequence_suspend_and_return` will co_return after that,
    //  both coroutine will reach *final suspended* state
    REQUIRE(fc.done());
    REQUIRE(fs.done());
    REQUIRE(status == 2);

    // try {
    //     fc.destroy();
    // } catch (...) {
    //     puts("?>?");
    // }
    // printf("%p %p \n", ptr->factivate, ptr->fdestroy);
}

// use like a generator
auto sequence_yield_once(status_t value = 0) -> sequence<status_t> {
    co_yield value;
    co_return;
}
auto use_sequence_yield_once(status_t& ref) -> no_return {
    // clang-format off
    for co_await(auto v : sequence_yield_once(0xBEE))
        ref = v;
    // clang-format on
};

auto coro_sequence_yield_once_test() {
    status_t storage = -1;
    use_sequence_yield_once(storage);
    REQUIRE(storage == 0xBEE); // storage will receive value
};

auto sequence_suspend_with_await(frame& manual_resume) -> sequence<status_t> {
    status_t value = 0;
    co_yield value = 1;
    co_await manual_resume; // use `co_await` instead of `co_yield`
    co_yield value = 2;
}
auto use_sequence_yield_suspend_yield_1(status_t& ref, frame& fh) -> frame {
    // clang-format off
    for co_await(auto v : sequence_suspend_with_await(fh))
        ref = v;
    // clang-format on
};

auto coro_sequence_suspend_using_await_test() {
    // see also: `coro_sequence_frame_status_test`
    frame fs{};

    status_t storage = -1;
    auto fc = use_sequence_yield_suspend_yield_1(storage, fs);
    REQUIRE(fs.done() == false); // we didn't finished iteration
    REQUIRE(storage == 1);       // co_yield value = 1

    // continue after co_await manual_resume;
    fs.resume();
    REQUIRE(storage == 2); // co_yield value = 2;
    REQUIRE(fs.done());
    REQUIRE(fc.done());
    // fc.destroy();
}

auto sequence_suspend_with_yield(frame& manual_resume) -> sequence<status_t> {
    status_t value = 0;
    co_yield value = 1;
    co_yield manual_resume; // use `co_yield` instead of `co_await`
    co_yield value = 2;
};
auto use_sequence_yield_suspend_yield_2(status_t& ref, frame& fh) -> frame {
    // clang-format off
    for co_await(auto v : sequence_suspend_with_yield(fh))
        ref = v;
    // clang-format on
};

auto coro_sequence_suspend_using_yield_test() {
    // see also: `coro_sequence_frame_status_test`
    frame fs{};

    status_t storage = -1;
    auto fc = use_sequence_yield_suspend_yield_2(storage, fs);
    REQUIRE(fs.done() == false); // we didn't finished iteration
    REQUIRE(storage == 1);       // co_yield value = 1

    // continue after co_await manual_resume;
    fs.resume();
    REQUIRE(storage == 2); // co_yield value = 2;
    REQUIRE(fs.done());
    REQUIRE(fc.done());
    fc.destroy();
}

auto use_sequence_yield_suspend_yield_final(status_t* ptr, frame& fh) -> frame {
    auto on_return = gsl::finally([=]() {
        *ptr = 0xDEAD; // set the value in destruction phase
    });
    // clang-format off
    for co_await(auto v : sequence_suspend_with_yield(fh))
        *ptr = v;
    // clang-format on
};

auto coro_sequence_destroy_when_suspended_test() {
    status_t storage = -1;
    frame fs{}; // frame of sequence

    auto fc = use_sequence_yield_suspend_yield_final(&storage, fs);
    REQUIRE(fs.done() == false); // it is suspended. of course false
    REQUIRE(storage == 1);       // co_yield value = 1

    // manual destruction of the coroutine frame
    fc.destroy();
    // as mentioned in `sequence_suspend_and_return`,
    //  destruction of `fs` will lead access violation
    //  when its caller is resumed.
    // however, it is still safe to destroy caller frame and
    //  it won't be a resource leak
    //  since the step also destroys frame of sequence
    REQUIRE(storage == 0xDEAD); // notice the gsl::finally
}

#if __has_include(<CppUnitTest.h>)

class coro_sequence_no_yield : public TestClass<coro_sequence_no_yield> {
    TEST_METHOD(test_coro_sequence_no_yield) {
        coro_sequence_no_yield_test();
    }
};
class coro_sequence_frame_status
    : public TestClass<coro_sequence_frame_status> {
    TEST_METHOD(test_coro_sequence_frame_status) {
        coro_sequence_frame_status_test();
    }
};
class coro_sequence_yield_once : public TestClass<coro_sequence_yield_once> {
    TEST_METHOD(test_coro_sequence_yield_once) {
        coro_sequence_yield_once_test();
    }
};
class coro_sequence_suspend_using_await
    : public TestClass<coro_sequence_suspend_using_await> {
    TEST_METHOD(test_coro_sequence_suspend_using_await) {
        coro_sequence_suspend_using_await_test();
    }
};
class coro_sequence_suspend_using_yield
    : public TestClass<coro_sequence_suspend_using_yield> {
    TEST_METHOD(test_coro_sequence_suspend_using_yield) {
        coro_sequence_suspend_using_yield_test();
    }
};
class coro_sequence_destroy_when_suspended
    : public TestClass<coro_sequence_destroy_when_suspended> {
    TEST_METHOD(test_coro_sequence_destroy_when_suspended) {
        coro_sequence_destroy_when_suspended_test();
    }
};
#endif
