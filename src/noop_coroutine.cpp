// Microsoft STL, C++ 17 (Coroutines TS)
#include <experimental/coroutine>

#if defined(_MSC_VER)
namespace std::experimental {
// 17.12.4, no-op coroutines
struct noop_coroutine_promise {};

// STRUCT noop_coroutine_handle
using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

// 17.12.4.3
noop_coroutine_handle noop_coroutine() noexcept;

// STRUCT coroutine_handle<noop_coroutine_promise>
template <>
struct coroutine_handle<noop_coroutine_promise> : public coroutine_handle<void> {
  private:
    friend noop_coroutine_handle noop_coroutine() noexcept;
};

struct noop_coroutine_return_type {
    struct promise_type final {
        constexpr suspend_always initial_suspend() noexcept {
            return {};
        };
        constexpr suspend_always final_suspend() noexcept {
            return {};
        };
        noop_coroutine_return_type get_return_object() {
            return noop_coroutine_return_type{*this};
        };
        void unhandled_exception() noexcept {
        }
        void return_void() noexcept {
        }
    };

  public:
    explicit noop_coroutine_return_type(promise_type& p) noexcept
        : handle{coroutine_handle<promise_type>::from_promise(p)} {
    }
    ~noop_coroutine_return_type() noexcept {
        handle.destroy();
    }
    noop_coroutine_return_type(const noop_coroutine_return_type&) = delete;
    noop_coroutine_return_type(noop_coroutine_return_type&&) = delete;
    noop_coroutine_return_type& operator=(const noop_coroutine_return_type&) = delete;
    noop_coroutine_return_type& operator=(noop_coroutine_return_type&&) = delete;

  public:
    coroutine_handle<promise_type> handle;
};

auto spawn_noop_coroutine() -> noop_coroutine_return_type {
    while (true)
        co_await suspend_always{};
    co_return;
}

noop_coroutine_handle noop_coroutine() noexcept {
    static noop_coroutine_return_type noop = spawn_noop_coroutine();
    auto coro = coroutine_handle<void>::from_address(noop.handle.address());
    return noop_coroutine_handle{coro};
}
} // namespace std::experimental
#endif
