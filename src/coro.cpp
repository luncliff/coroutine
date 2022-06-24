// Microsoft STL, C++ 17 (Coroutines TS)
#include "coro.hpp"

#if defined(_MSC_VER) && (__cplusplus < 202000L) && (__cplusplus >= 201700L) // C++ 17 only
#if !__builtin_coro_noop

namespace std::experimental {

noop_coroutine_handle::coroutine_handle(coroutine_handle<void> handle) noexcept : coroutine_handle<void>{handle} {
}

class noop_return_type final {
  public:
    struct promise_type final {
        constexpr suspend_never initial_suspend() noexcept {
            return {};
        }
        constexpr suspend_always final_suspend() noexcept {
            return {};
        }
        void unhandled_exception() noexcept {
        }
        constexpr void return_void() noexcept {
        }
        noop_return_type get_return_object() noexcept {
            return noop_return_type{*this};
        }
    };

  public:
    explicit noop_return_type(promise_type& p) noexcept;
    ~noop_return_type() noexcept;

  public:
    coroutine_handle<void> handle;
};

noop_return_type::noop_return_type(promise_type& p) noexcept : handle{coroutine_handle<promise_type>::from_promise(p)} {
}

noop_return_type::~noop_return_type() noexcept {
    handle.destroy();
}

static noop_return_type spawn_noop_coroutine() noexcept {
    while (true)
        co_await suspend_always{};
    co_return;
}

noop_coroutine_handle noop_coroutine() noexcept {
    static const noop_return_type noop = spawn_noop_coroutine();
    return noop_coroutine_handle{noop.handle};
}

} // namespace std::experimental
#endif // !__builtin_coro_noop
#endif
