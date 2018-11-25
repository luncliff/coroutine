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

#include "./adapter.h"

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
extern void register_thread(thread_id_t thread_id) noexcept(false);
extern void forget_thread(thread_id_t thread_id) noexcept(false);

thread_id_t current_thread_id() noexcept
{
    // prevent size problem
    static_assert(sizeof(pthread_t) == sizeof(uint64_t));
    static_assert(sizeof(pthread_t) == sizeof(thread_id_t));
    // redirect to `pthread_self`
    return static_cast<thread_id_t>(pthread_self());
}

namespace itr1 // internal 1
{
class queue_mgmt_t final
{
  public:
    queue_mgmt_t() noexcept(false) { register_thread(current_thread_id()); }
    ~queue_mgmt_t() noexcept { forget_thread(current_thread_id()); }
};
thread_local queue_mgmt_t life1{};

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

} // namespace itr1
