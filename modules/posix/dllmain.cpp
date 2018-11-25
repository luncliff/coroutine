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

extern void setup_messaging() noexcept(false);
extern void teardown_messaging() noexcept(false);
extern void add_messaging_thread(thread_id_t tid) noexcept(false);
extern void remove_messaging_thread(thread_id_t tid) noexcept(false);

namespace itr1 // internal 1
{
LIB_PROLOGUE void load_callback() noexcept(false)
{
    // ... process must setup messaging for thread ...
    setup_messaging();
}

LIB_EPILOGUE void unload_callback() noexcept(false)
{
    // ... process must teardown messaging for thread ...
    teardown_messaging();
}

struct life_event_hook_t
{
  public:
    life_event_hook_t()
    {
        add_messaging_thread(
            static_cast<thread_id_t>((uint64_t)pthread_self()));
        // std::printf("thread ctor %lu \n", pthread_self());
    }
    ~life_event_hook_t()
    {
        remove_messaging_thread(
            static_cast<thread_id_t>((uint64_t)pthread_self()));
        // std::printf("thread dtor %lu \n", pthread_self());
    }

  public:
    auto get_id() const noexcept
    {
        return static_cast<thread_id_t>((uint64_t)pthread_self());
    }
};

thread_local life_event_hook_t hook{};

life_event_hook_t& this_thread() noexcept { return hook; }

} // namespace itr1

thread_id_t current_thread_id() noexcept
{
    // prevent size problem
    static_assert(sizeof(pthread_t) == sizeof(uint64_t));
    static_assert(sizeof(pthread_t) == sizeof(thread_id_t));

    auto& hook = itr1::this_thread();

    // redirect to `pthread_self`
    return hook.get_id();
}
