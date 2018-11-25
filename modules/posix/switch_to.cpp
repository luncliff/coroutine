// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/frame.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

#include <cassert>
#include <csignal>
#include <future>
#include <system_error>

#include "./adapter.h"

using namespace std;
using namespace std::experimental;

bool peek_switched( // ... better name?
    std::experimental::coroutine_handle<void>& coro) noexcept(false)
{
    message_t msg{};
    if (peek_message(msg) == true)
    {
        coro = coroutine_handle<void>::from_address(msg.ptr);
        return true;
    }
    return false;
}

static_assert(std::is_nothrow_move_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_move_constructible_v<switch_to> == false);
static_assert(std::is_nothrow_copy_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_copy_constructible_v<switch_to> == false);

struct switch_to_posix
{
    thread_id_t thread_id{};
    void* work{};
};
static_assert(sizeof(switch_to_posix) <= sizeof(switch_to));

auto* for_posix(switch_to* s) noexcept
{
    return reinterpret_cast<switch_to_posix*>(s);
}
auto* for_posix(const switch_to* s) noexcept
{
    return reinterpret_cast<const switch_to_posix*>(s);
}

switch_to::switch_to(uint64_t target) noexcept(false) : u64{}
{
    u64[0] = 0; // prevent unused variable warning
    auto* sw = for_posix(this);
    sw->thread_id = static_cast<thread_id_t>(target);
}

switch_to::~switch_to() noexcept
{
    auto* sw = for_posix(this);
    assert(sw->work == nullptr);
}

bool switch_to::ready() const noexcept
{
    const auto* task = for_posix(this);
    bool is_ready = false;

    // for background work,
    if (task->thread_id == thread_id_t{})
    {
        // always false
        is_ready = false;
    }
    else
    {
        is_ready = (task->thread_id == current_thread_id());
    }

    std::printf("switch_to::ready %d \n", (is_ready) ? 1 : -1);
    // already in the target thread?
    return is_ready;
}

//
// !!! notice that this is not atomic !!!
//
//  See 'worker_group.cpp' for the detail.
//
extern thread_id_t unknown_worker_id;

void switch_to::suspend( //
    std::experimental::coroutine_handle<void> coro) noexcept(false)
{
    auto* sw = for_posix(this);

    // submit to specific thread
    message_t msg{};
    msg.ptr = coro.address();

    const thread_id_t worker_id =
        (sw->thread_id != thread_id_t{}) ? sw->thread_id : unknown_worker_id;

    std::printf("switch_to::suspend %p >> %lx \n", msg.ptr, worker_id);

    post_message(worker_id, msg);
}

void switch_to::resume() noexcept
{
    const auto* sw = for_posix(this);

    std::printf(
        "switch_to::resume %lx %lx \n", sw->thread_id, current_thread_id());

    if (sw->thread_id != thread_id_t{})
        // check thread id
        assert(sw->thread_id == current_thread_id());
}
