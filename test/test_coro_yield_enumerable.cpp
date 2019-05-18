//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

class coro_enumerable_no_yield_test : public test_adapter {
    static auto yield_never() -> enumerable<int> {
        // co_return is necessary so compiler can notice that
        // this is a coroutine when there is no co_yield.
        co_return;
    };

  public:
    void on_test() override {
        auto count = 0u;
        for (auto v : yield_never())
            count += 1;
        expect_true(count == 0);
    }
};

class coro_enumerable_yield_once_test : public test_adapter {
    static auto yield_once(int value = 0) -> enumerable<int> {
        co_yield value;
        co_return;
    };

  public:
    void on_test() override {
        auto count = 0u;
        for (auto v : yield_once()) {
            expect_true(v == 0);
            count += 1;
        }
        expect_true(count > 0);
    }
};

class coro_enumerable_iterator_test : public test_adapter {
    static auto yield_once(int value = 0) -> enumerable<int> {
        co_yield value;
        co_return;
    };

  public:
    void on_test() override {
        auto count = 0u;
        auto g = yield_once();
        auto b = g.begin();
        auto e = g.end();
        for (auto it = b; it != e; ++it) {
            expect_true(*it == 0);
            count += 1;
        }
        expect_true(count > 0);
    }
};

class coro_enumerable_after_move_test : public test_adapter {
    static auto yield_once(int value = 0) -> enumerable<int> {
        co_yield value;
        co_return;
    };

  public:
    void on_test() override {
        auto count = 0u;
        auto g = yield_once();
        auto m = move(g);
        // g lost its handle. so it is not iterable anymore
        expect_true(g.begin() == g.end());
        for (auto v : g)
            fail_with_message("null generator won't go into loop");

        for (auto v : m) {
            expect_true(v == 0);
            count += 1;
        }
        expect_true(count > 0);
    }
};

class coro_enumerable_accumulate_test : public test_adapter {
    static auto yield_until_zero(int n) -> enumerable<int> {
        while (n-- > 0)
            co_yield n;
    };

  public:
    void on_test() override {
        auto g = yield_until_zero(10);
        auto total = accumulate(g.begin(), g.end(), 0u);
        expect_true(total == 45); // 0 - 10
    }
};

class coro_enumerable_max_element_test : public test_adapter {
    template <size_t Sz>
    auto yield_with_container(array<uint32_t, Sz>& storage)
        -> enumerable<uint32_t> {
        for (auto& ref : storage)
            co_yield ref;
    };

  public:
    void on_test() override {
        // since generator is not a container,
        //  using max_element (or min_element) function on it
        //  will return invalid iterator
        auto id = 15u;
        array<uint32_t, 10> storage{};
        for (auto& e : storage)
            e = id--; // [15, 14, 13 ...
                      // so the first element will hold the largest number

        auto g = yield_with_container(storage);
        auto it = max_element(g.begin(), g.end());

        // after iteration is finished (`co_return`),
        // the iterator will hold nullptr.
        expect_true(it.operator->() == nullptr);

        // so referencing it will lead to access violation
        // REQUIRE(*it == 15);
    }
};
