//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace std::experimental;
using namespace coro;

class coroutine_handle_move_test : public test_adapter {
  public:
    void *p1, *p2;
    coroutine_handle<void> c1{}, c2{};

  public:
    void on_setup() override {
        // libc++ requires the parameter must be void* type
        p1 = &c1, p2 = &c2;
        c1 = coroutine_handle<void>::from_address(p1);
        c2 = coroutine_handle<void>::from_address(p2);
    }
    void on_test() override {
        expect_true(c1.address() == p1);
        expect_true(c2.address() == p2);

        c2 = move(c1);
        // expect the address is moved to c2
        expect_true(c1.address() == p1);
        expect_true(c2.address() == p1);
    };
};

class coroutine_handle_swap_test : public test_adapter {
  public:
    void *p1, *p2;
    coroutine_handle<void> c1{}, c2{};

  public:
    void on_setup() override {
        // libc++ requires the parameter must be void* type
        p1 = &c1, p2 = &c2;
        c1 = coroutine_handle<void>::from_address(p1);
        c2 = coroutine_handle<void>::from_address(p2);
    }
    void on_test() override {
        expect_true(c1.address() != nullptr);
        expect_true(c2.address() != nullptr);

        swap(c1, c2);
        expect_true(c1.address() == p2);
        expect_true(c2.address() == p1);
    }
};
