/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief  Shows how to work with VC++ <future> <experimental/resumable>
 */
#undef NDEBUG
#include <cassert>
#include <gsl/gsl>

#if defined(_MSC_VER)
// !!! this undef will be ignored !!!
#undef _RESUMABLE_FUNCTIONS_SUPPORTED
// so if <future> comes before <coroutine/frame.h>,
// _Future_awaiter will make a compiler error for `coroutine_handle`
#include <coroutine/return.h> // import <coroutine/frame.h>
#include <future>
#else
// If you are not in VC++, you can place <future> anywhere
#include <future>

#include <coroutine/return.h> // import <coroutine/frame.h>
#endif

using namespace std;

auto set_after_resume(std::promise<int>& p, int lhs, int rhs)
    -> coro::passive_frame_t {
    co_await suspend_never{};
    p.set_value(lhs + rhs);
}

int main(int, char* []) {
    std::promise<int> p{};
    auto f = p.get_future();

    // after invoke, the future is not ready
    auto task = set_after_resume(p, 1, 2);
    assert(f.wait_for(std::chrono::seconds{}) == std::future_status::timeout);

    // after resume, the routine will set_value
    task();
    assert(f.wait_for(std::chrono::seconds{}) == std::future_status::ready);
    assert(f.get() == 3);
    return 0;
}

// test compatibility with old namespace
namespace std::experimental {

static_assert(sizeof(coroutine_handle<void>) == sizeof(void*));

} // namespace std::experimental
