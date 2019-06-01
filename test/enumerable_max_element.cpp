//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

using namespace coro;

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
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return coro_enumerable_max_element_test();
}

#elif __has_include(<CppUnitTest.h>)
class coro_enumerable_max_element
    : public TestClass<coro_enumerable_max_element> {
    TEST_METHOD(test_coro_enumerable_max_element) {
        coro_enumerable_max_element_test();
    }
};
#endif
