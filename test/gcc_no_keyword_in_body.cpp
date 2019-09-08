//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//
//  Note
//    This is a test code for GCC C++ Coroutines
//
//    A coroutine function returns a type which fulfills
//    the Coroutine Promise Requirement,
//    but it forgot to use `co_await` or `co_return` in its definition
//
#include <cstdio>

#include "coro.h"

using namespace std;
using namespace std::experimental;

class promise_nn {
  public:
    auto initial_suspend() noexcept {
        return suspend_never{};
    }
    auto final_suspend() noexcept {
        return suspend_never{};
    }
    void unhandled_exception() noexcept(false) {
    }
};

class forget_frame {
  public:
    class promise_type final : public promise_nn {
      public:
        void return_void() noexcept {
        }
        auto get_return_object() noexcept -> forget_frame {
            return {this};
        }
        static auto get_return_object_on_allocation_failure() noexcept
            -> forget_frame {
            return {nullptr};
        }
    };

  private:
    forget_frame(const promise_type*) noexcept {
        // the type truncates all given info about its frame
    }

  public:
    // gcc-10 requires the type to be default constructible
    forget_frame() noexcept = default;
};

auto no_keyword_coroutine(int argc) -> forget_frame {
    // use coroutine's return type, but no `co_await` or `co_return`
    printf("%d\n", argc);
}

int main(int argc, char* []) {
    no_keyword_coroutine(argc);
    return 0;
}
