//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
#include <coroutine/frame.h>

#include <coroutine/concrt.h>
#include <coroutine/return.h>
#include <coroutine/yield.hpp>

#include <gsl/gsl>

#include <array>
#include <atomic>
#include <future>

using namespace std;

extern void expect_true(bool cond);
extern void fail_with_message(string&& msg);

class test_adapter {
  public:
    test_adapter() noexcept = default;
    virtual ~test_adapter() noexcept = default;

    test_adapter(const test_adapter&) = delete;
    test_adapter(test_adapter&&) = delete;
    test_adapter& operator=(const test_adapter&) = delete;
    test_adapter& operator=(test_adapter&&) = delete;

  public:
    virtual void on_setup(){};
    virtual void on_teardown(){};
    virtual void on_test() = 0;
};
