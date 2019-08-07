//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//
#include <gsl/gsl>

template <typename P>
struct coroutine_handle {};

struct suspend_never {
  public:
    constexpr bool await_ready() const noexcept {
        return true;
    }
    constexpr void await_suspend(coroutine_handle<void>) noexcept {
    }
    constexpr void await_resume() noexcept {
    }
};

struct promise_basic {
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    auto final_suspend() noexcept {
        return suspend_never{};
    }
    void unhandled_exception() noexcept {
    }
};

// clang-format off

template<typename T, typename R = void>
concept bool awaitable = requires(T a, coroutine_handle<void> h) {
    { a.await_ready() } -> bool;
    { a.await_suspend(h) } -> void;
    { a.await_resume() } -> R;
};

template<typename P>
concept bool promise_requirement_basic = requires(P p) {
    { p.initial_suspend() } -> awaitable;
    { p.final_suspend() } -> awaitable;
    { p.unhandled_exception() } -> void;
};

// clang-format on

struct promise_void : public promise_basic {
    void return_void() noexcept {
    }
    auto get_return_object() noexcept {
        return this;
    }
};

// clang-format off

template<typename T>
concept bool not_void = !std::is_same_v<T, void>;

template<promise_requirement_basic P>
concept bool promise_return_void = requires(P p) {
    { p.return_void() } -> void;
    { p.get_return_object() } -> not_void;
};

// clang-format on

template <not_void T>
struct promise_value : public promise_basic {
    using value_type = T;

    void return_value(T&& v) noexcept {
    }
    auto get_return_object() noexcept {
        return this;
    }
};

// clang-format off
template<promise_requirement_basic P, not_void T>
concept bool promise_return_value = requires(P p, T&& v) {
    { p.return_value(std::move(v)) } -> void;
    { p.get_return_object() } -> not_void;
};
// clang-format on

// clang-format off
// todo: combination with `std::enable_if`
template<promise_requirement_basic P, not_void T>
concept bool promise_requirement_return = 
    promise_return_void<P> || promise_return_value<P,T>;
// clang-format on

// clang-format off
template<typename T>
concept bool has_promise_type = requires(){
    typename T::promise_type;
};
// clang-format on

struct return_1 final {
    using promise_type = promise_void;

  public:
    return_1(promise_type*) noexcept {
    }
};

#define REQUIRE(expr)                                                          \
    if ((expr) == false)                                                       \
        return __LINE__;

int main(int, char* []) {
    // not void
    REQUIRE(not_void<void> == false);
    REQUIRE(not_void<int>);

    // requirement basic
    REQUIRE(promise_requirement_basic<void> == false);
    REQUIRE(promise_requirement_basic<promise_basic>);

    // requirement void
    REQUIRE(promise_return_void<void> == false);
    REQUIRE(promise_requirement_basic<promise_void>);
    REQUIRE(promise_return_void<promise_void>);

    // requirement return
    using promise_1 = promise_void;
    using promise_2 = promise_value<std::string>;
    static_assert(promise_requirement_return<promise_1, void>);
    static_assert(promise_requirement_return<promise_2, promise_2::value_type>);

    // has_promise_type
    REQUIRE(has_promise_type<int> == false);
    REQUIRE(has_promise_type<return_1>);

    return 0;
}

// TEST_CASE("requirement value") {
//     // error:   template argument 1 is invalid
//     // error:   template constraint failure
//     // note:    constraints not satisfied
//     // note:    within 'template<class T> concept const bool not_void<T>
//     [with T
//     // = void]' REQUIRE_FALSE(promise_return_value<promise_value<void>,
//     void>);
//     // however, this constraint is passed
//     static_assert(!promise_return_value<void, void>);
//     using promise_type = promise_value<std::string>;
//     REQUIRE(promise_requirement_basic<promise_type>);
//     REQUIRE(promise_return_value<promise_type, promise_type::value_type>);
// }
