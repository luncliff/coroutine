//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/yield.hpp>

#include <catch2/catch.hpp>

#include <gsl/gsl>

using namespace std;
using namespace std::experimental;
using namespace coro;

auto enumerable_yield_never() -> enumerable<int> {
    // co_return is necessary so compiler can notice that
    // this is a coroutine when there is no co_yield.
    co_return;
};

TEST_CASE("generator yield never", "[generic]") {
    auto count = 0u;
    for (auto v : enumerable_yield_never())
        count += 1;
    REQUIRE(count == 0);
}

auto enumerable_yield_once(int value = 0) -> enumerable<int> {
    // co_return is necessary so compiler can notice that
    // this is a coroutine when there is no co_yield.
    co_yield value;
    co_return;
};

TEST_CASE("generator yield once", "[generic]") {
    auto count = 0u;
    SECTION("range for") {
        for (auto v : enumerable_yield_once()) {
            REQUIRE(v == 0);
            count += 1;
        }
    }
    SECTION("iterator") {
        auto g = enumerable_yield_once();
        auto b = g.begin();
        auto e = g.end();
        for (auto it = b; it != e; ++it) {
            REQUIRE(*it == 0);
            count += 1;
        }
    }
    SECTION("move semantics") {
        auto g = enumerable_yield_once();
        auto m = std::move(g);
        // g lost its handle. so it is not iterable anymore
        REQUIRE(g.begin() == g.end());
        for (auto v : g)
            FAIL("null generator won't go into loop");

        for (auto v : m) {
            REQUIRE(v == 0);
            count += 1;
        }
    }
    REQUIRE(count > 0);
}

auto enumerable_yield_until_zero(int n) -> enumerable<int> {
    while (n-- > 0)
        co_yield n;
};

TEST_CASE("generator iterator accumulate", "[generic]") {
    auto g = enumerable_yield_until_zero(10);
    auto total = accumulate(g.begin(), g.end(), 0u);
    REQUIRE(total == 45); // 0 - 10
}

template <typename T, size_t Sz>
auto enumerable_yield_with_container(array<T, Sz>& storage) -> enumerable<T> {
    for (auto& ref : storage)
        co_yield ref;
};

TEST_CASE("generator iterator max_element", "[generic]") {
    // since generator is not a container,
    //  using max_element (or min_element) function on it
    //  will return invalid iterator
    auto id = 15u;
    array<uint32_t, 10> storage{};
    for (auto& e : storage)
        e = id--; // [15, 14, 13 ...
                  // so the first element will hold the largest number

    auto g = enumerable_yield_with_container(storage);
    auto it = max_element(g.begin(), g.end());

    // after iteration is finished (`co_return`),
    // the iterator will hold nullptr.
    REQUIRE(it.operator->() == nullptr);

    // so referencing it will lead to access violation
    // REQUIRE(*it == 15);
}

auto sequence_yield_never() -> sequence<int> {
    co_return;
}

TEST_CASE("async generator yield never", "[generic]") {
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
    REQUIRE(storage == -1);
}

auto sequence_suspend_and_return(frame& fh) -> sequence<int> {
    co_yield fh; // save current coroutine to the `frame`
                 // the `frame` type inherits `suspend_always`
    co_return;   // return without yield
}

TEST_CASE("async generator suspend and return", "[generic]") {
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
    REQUIRE(storage == -1);
    // however, when the sequence coroutine is resumed,
    // its caller will continue on ...
    REQUIRE(fs.address() != nullptr);
    fs.resume();        // sequence coroutine will be resumed
                        //  and it will resume `use_async_gen`.
    REQUIRE(fs.done()); //  since `use_async_gen` will return after that,
    REQUIRE(fc.done()); //  both coroutine will reach *final suspended* state
    REQUIRE(storage == 2);
}

auto sequence_yield_once(int value = 0) -> sequence<int> {
    co_yield value;
    co_return;
}

TEST_CASE("async generator yield once", "[generic]") {
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
    REQUIRE(storage == 0xBEE); // storage will receive value
}

auto sequence_yield_suspend_yield_1(frame& manual_resume) -> sequence<int> {
    auto value = 0;
    co_yield value = 1;
    co_await manual_resume;
    co_yield value = 2;
}
auto sequence_yield_suspend_yield_2(frame& manual_resume) -> sequence<int> {
    auto value = 0;
    co_yield value = 1;
    co_yield manual_resume; // use `co_yield` instead of `co_await`
    co_yield value = 2;
}

TEST_CASE("async generator yield suspend yield", "[generic]") {
    frame fc{}, fs{}; // frame of caller, frame of sequence
    auto no_frame_leak = gsl::finally([&]() {
        // see `sequence_suspend_and_return`'s test case
        if (fc.address() != nullptr)
            fc.destroy();
    });

    int storage = -1;
    SECTION("case 1") {
        auto use_async_gen = [](int& ref, frame& fh) -> frame {
            // clang-format off
            for co_await(auto v : sequence_yield_suspend_yield_1(fh))
                ref = v;
            // clang-format on
        };
        fc = use_async_gen(storage, fs);
    }
    SECTION("case 2") {
        auto use_async_gen = [](int& ref, frame& fh) -> frame {
            // clang-format off
            for co_await(auto v : sequence_yield_suspend_yield_2(fh))
                ref = v;
            // clang-format on
        };
        fc = use_async_gen(storage, fs);
    }
    REQUIRE(fc.done() == false); // we didn't finished iteration
    REQUIRE(fs.done() == false); // co_await manual_resume
    REQUIRE(storage == 1);       // co_yield value = 1
    fs.resume();                 // continue after co_await manual_resume;
    REQUIRE(storage == 2);       // co_yield value = 2;
    REQUIRE(fc.done());          // both are finished
    REQUIRE(fs.done());
}

TEST_CASE("async generator destroy when suspended", "[generic]") {
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

    REQUIRE(fs.done() == false); // it is suspended. of course false !
    REQUIRE(storage == 1);       // co_yield value = 1
    fc.destroy();
    // as mentioned in `sequence_suspend_and_return`, destruction of `fs`
    //  will lead access violation in caller frame destruction.
    // however, it is still safe to destroy caller frame
    //  and it won't be a resource leak
    //  since the step also destroys frame of sequence
    REQUIRE(storage == 0xDEAD); // see gsl::finally
}
