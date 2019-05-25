//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

auto yield_never() -> enumerable<int> {
    // co_return is necessary so compiler can notice that
    // this is a coroutine when there is no co_yield.
    co_return;
};
auto coro_enumerable_no_yield_test() {
    auto count = 0u;
    for (auto v : yield_never()) {
        v = count; // code to suppress C4189
        count += 1;
    }
    REQUIRE(count == 0);
}

auto yield_once(int value = 0) -> enumerable<int> {
    co_yield value;
    co_return;
};
auto coro_enumerable_yield_once_test() {

    auto count = 0u;
    for (auto v : yield_once()) {
        REQUIRE(v == 0);
        count += 1;
    }
    REQUIRE(count > 0);
};

auto coro_enumerable_iterator_test() {
    auto count = 0u;
    auto g = yield_once();
    auto b = g.begin();
    auto e = g.end();
    for (auto it = b; it != e; ++it) {
        REQUIRE(*it == 0);
        count += 1;
    }
    REQUIRE(count > 0);
}

auto coro_enumerable_after_move_test() {
    auto count = 0u;
    auto g = yield_once();
    auto m = move(g);
    // g lost its handle. so it is not iterable anymore
    REQUIRE(g.begin() == g.end());
    for (auto v : g) {
        v = count; // code to suppress C4189
        FAIL_WITH_MESSAGE("null generator won't go into loop"s);
    }

    for (auto v : m) {
        REQUIRE(v == 0);
        count += 1;
    }
    REQUIRE(count > 0);
}

auto yield_until_zero(int n) -> enumerable<int> {
    while (n-- > 0)
        co_yield n;
};
auto coro_enumerable_accumulate_test() {
    auto g = yield_until_zero(10);
    auto total = accumulate(g.begin(), g.end(), 0u);
    REQUIRE(total == 45); // 0 - 10
}

template <size_t Sz>
auto yield_with_container(array<uint32_t, Sz>& storage)
    -> enumerable<uint32_t> {
    for (auto& ref : storage)
        co_yield ref;
};
auto coro_enumerable_max_element_test() {
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
    REQUIRE(it.operator->() == nullptr);

    // so referencing it will lead to access violation
    // REQUIRE(*it == 15);
}

#if __has_include(<catch2/catch.hpp>)
TEST_CASE("generator no yield", "[yield]") {
    coro_enumerable_no_yield_test();
}
TEST_CASE("generator yield once", "[yield]") {
    coro_enumerable_yield_once_test();
}
TEST_CASE("generator iterator", "[yield]") {
    coro_enumerable_iterator_test();
}
TEST_CASE("generator after move", "[yield]") {
    coro_enumerable_after_move_test();
}
TEST_CASE("generator accumulate", "[yield]") {
    coro_enumerable_accumulate_test();
}
TEST_CASE("generator max_element", "[yield]") {
    coro_enumerable_max_element_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_enumerable_no_yield : public TestClass<coro_enumerable_no_yield> {
    TEST_METHOD(test_coro_enumerable_no_yield) {
        coro_enumerable_no_yield_test();
    }
};
class coro_enumerable_yield_once
    : public TestClass<coro_enumerable_yield_once> {
    TEST_METHOD(test_coro_enumerable_yield_once) {
        coro_enumerable_yield_once_test();
    }
};
class coro_enumerable_iterator : public TestClass<coro_enumerable_iterator> {
    TEST_METHOD(test_coro_enumerable_iterator) {
        coro_enumerable_iterator_test();
    }
};
class coro_enumerable_accumulate
    : public TestClass<coro_enumerable_accumulate> {
    TEST_METHOD(test_coro_enumerable_accumulate) {
        coro_enumerable_accumulate_test();
    }
};
class coro_enumerable_max_element
    : public TestClass<coro_enumerable_max_element> {
    TEST_METHOD(test_coro_enumerable_max_element) {
        coro_enumerable_max_element_test();
    }
};

#endif
