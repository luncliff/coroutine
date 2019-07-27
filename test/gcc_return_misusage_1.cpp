//
//  https://github.com/iains/gcc-cxx-coroutines/blob/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines/coro.h
//
#include <cstdio>
#include <string>

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
        coroutine_handle<void>& ref = *this;
        ref = coroutine_handle<promise_type>::from_promise(*p);
    }

  public:
    // gcc-10 requires the type to be default constructible
    preserve_frame() noexcept = default;
};

using namespace std;
using namespace std::experimental;

auto assign_and_return(string& result) -> preserve_frame {
    // use coroutine's return type, but no `co_await` or `co_return`
    result = __FUNCTION__;
}

int main(int, char* []) {
    string result{};

    auto frame = assign_and_return(result);
    if (frame.address() != nullptr)
        return 1;

    return 0;
}
