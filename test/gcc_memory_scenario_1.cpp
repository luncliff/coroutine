//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//
//  Note
//    This is a test code for GCC C++ Coroutines
//
//    Memory allocation scenario - 1
//      Provides the function to handle allocation failure
//
#include <cstdio>

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
        }

        auto get_return_object() noexcept -> preserve_frame {
            puts(__FUNCTION__);
            return preserve_frame{this};
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> preserve_frame {
            puts(__FUNCTION__);
            return preserve_frame{nullptr};
        }
    };

  private:
    explicit preserve_frame(promise_type* p) noexcept
        : coroutine_handle<void>{} {

        printf("%s: %p\n", __FUNCTION__, p);
        if (p == nullptr)
            return;

        coroutine_handle<void>& ref = *this;
        ref = coroutine_handle<promise_type>::from_promise(*p);
    }
};

auto store_after_await(const char** label) noexcept -> preserve_frame {
    co_await suspend_never{};
    *label = __FUNCTION__;
    co_return;
}

int main(int, char* []) {

    const char* label = nullptr;
    auto frame = store_after_await(&label);
    printf("after invoke: %p %s\n", frame.address(), label);

    if (frame.address() == nullptr)
        return __LINE__;

    frame.destroy();
    return 0;
}
