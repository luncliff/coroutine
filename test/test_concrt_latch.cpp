//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace std::literals;
using namespace coro;

class concrt_latch_wait_after_ready_test : public test_adapter {
    using latch_t = concrt::latch;

    static constexpr auto num_of_work = 10u;
    latch_t group{num_of_work};
    array<future<void>, num_of_work> contexts{};

    static void count_down_on_latch(latch_t* group) {
        // simply count down
        group->count_down();
    }

  public:
    void on_test() override {
        // general fork-join will be similar to this ...
        for (auto& f : contexts)
            f = async(launch::async, count_down_on_latch, addressof(group));

        group.wait();
        expect_true(group.is_ready());

        for (auto& f : contexts)
            f.wait();
    };
};

class concrt_latch_count_down_and_wait_test : public test_adapter {
    using latch_t = concrt::latch;

  public:
    void on_test() override {
        latch_t group{1};
        expect_true(group.is_ready() == false);

        group.count_down_and_wait();
        expect_true(group.is_ready());
        expect_true(group.is_ready()); // mutliple test is ok
    }
};

class concrt_latch_throws_when_negative_from_positive_test
    : public test_adapter {
    using latch_t = concrt::latch;

  public:
    void on_test() override {
        latch_t group{1};
        try {
            group.count_down(4);
        } catch (const underflow_error&) {
            return;
        }
        fail_with_message("expect exception");
    }
};
class concrt_latch_throws_when_negative_from_zero_test : public test_adapter {
    using latch_t = concrt::latch;

  public:
    void on_test() override {
        latch_t group{0};
        try {
            group.count_down(2);
        } catch (const underflow_error&) {
            return;
        }
        fail_with_message("expect exception");
    }
};
