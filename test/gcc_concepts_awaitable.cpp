
#include <gsl/gsl>

template <typename P>
struct coroutine_handle {};

// clang-format off

template<typename T, typename R = void>
concept bool awaitable = requires(T a, coroutine_handle<void> h) {
    { a.await_ready() } -> bool;
    { a.await_suspend(h) } -> void;
    { a.await_resume() } -> R;
};

// clang-format on

class public_await_functions {
  public:
    bool await_ready() const noexcept {
        return true;
    }
    void await_suspend(coroutine_handle<void>) noexcept(false) {
    }
    void await_resume() noexcept(false) {
    }
};

class private_await_functions : private public_await_functions {};

#define REQUIRE(expr)                                                          \
    if ((expr) == false)                                                       \
        return __LINE__;

int main(int, char* []) {
    // static assert
    REQUIRE(awaitable<int> == false);

    // access qualifier
    REQUIRE(awaitable<public_await_functions>);
    REQUIRE(awaitable<private_await_functions> == false);

    // reference
    public_await_functions target{};
    awaitable& a = target;
    REQUIRE(a.await_ready() == true);
    return 0;
}
