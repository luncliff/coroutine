// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/enumerable.hpp>
#include <coroutine/frame.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

#include <cassert>
#include <numeric>

#include <pthread.h>
#include <sys/types.h>

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

namespace coroutine
{
auto check_coroutine_available() noexcept -> unplug
{
    co_await std::experimental::suspend_never{};
}

auto check_generator_available() noexcept -> enumerable<uint16_t>
{
    uint16_t value = 4;
    co_yield value;
    co_yield value;
    co_return;
}

// this function is reserved for library initialization
// for now, it just checks some coroutine expressions
LIB_PROLOGUE void on_load_test(void*) noexcept(false)
{
    check_coroutine_available();
    auto g = check_generator_available();
    auto sum = std::accumulate(g.begin(), g.end(), 0u);
    assert(sum == 4 * 2);
}

} // namespace coroutine

extern void setup_indices() noexcept;
extern void teardown_indices() noexcept;
extern void register_thread(uint64_t thread_id) noexcept(false);
extern void forget_thread(uint64_t thread_id) noexcept(false);

namespace internal
{
class lifecycle_t
{
    static_assert(sizeof(pthread_t) == sizeof(uint64_t));

  public:
    const pthread_t thread_id;

    lifecycle_t() noexcept(false) : thread_id{pthread_self()}
    {
        auto tid = reinterpret_cast<uint64_t>(thread_id);
        register_thread(tid);
    }
    ~lifecycle_t() noexcept
    {
        auto tid = reinterpret_cast<uint64_t>(thread_id);
        forget_thread(tid);
    }
};
thread_local lifecycle_t _t_life{};

uint64_t current_thread_id() noexcept
{
    return reinterpret_cast<uint64_t>(_t_life.thread_id);
}

LIB_PROLOGUE void load_callback() noexcept(false)
{
    // ... process must setup indices for thread ...
    setup_indices();
}
LIB_EPILOGUE void unload_callback() noexcept(false)
{
    // ... process must teardown indices for thread ...
    teardown_indices();
}

} // namespace internal
