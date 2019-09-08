//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//
//  Note
//    This is a test code for GCC C++ Coroutines
//    Save current coroutine's frame using co_await operator
//
#include <cstdio>

// https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro.h
#include "coro.h"

using namespace std;
using namespace std::experimental;

class promise_return_preserve {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    auto final_suspend() noexcept {
        return suspend_always{};
    }
    void unhandled_exception() noexcept {
    }
};

class preserve_frame final : public coroutine_handle<void> {
  public:
    class promise_type final : public promise_return_preserve {
      public:
        void return_void() noexcept {
            // nothing to do because this is `void` return
        }
        auto get_return_object() noexcept -> preserve_frame {
            return preserve_frame{this};
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> preserve_frame {
            return preserve_frame{nullptr};
        }
    };

  private:
    explicit preserve_frame(promise_type* p) noexcept
        : coroutine_handle<void>{} {
        if (p == nullptr)
            return;
        coroutine_handle<void>& ref = *this;
        ref = coroutine_handle<promise_type>::from_promise(*p);
    }

  public:
    // gcc-10 requires the type to be default constructible
    preserve_frame() noexcept = default;
};

auto save_empty_frame(coroutine_handle<void>& coro) noexcept -> preserve_frame {
    co_await suspend_never{};
    puts("before assignment");
    coro = coroutine_handle<void>::from_address(nullptr);
    puts("after assignment");
    co_return;
}

int main(int, char* argv[]) {
    // make some invalid coroutine handle
    auto coro = coroutine_handle<void>::from_address(argv);
    puts("before invoke");

    auto frame = save_empty_frame(coro);
    puts("after invoke");

    if (frame)
        frame.destroy();

    if (coro.address() != nullptr)
        return __LINE__;

    return 0;
}
