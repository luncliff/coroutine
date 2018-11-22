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
    const auto tid = reinterpret_cast<uint64_t>(pthread_self());
    message_t msg{};
    if (peek_message(tid, msg) == true)
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

namespace internal
{
extern uint64_t current_thread_id() noexcept;
}

struct switch_to_posix
{
    uint64_t thread_id{};
    void* work{};
    uint32_t mark{};
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

switch_to::switch_to(uint32_t target) noexcept : u64{}
{
    constexpr uint32_t poison =
        std::numeric_limits<uint32_t>::max() - 0xFADE'BCFA;

    auto* self = for_posix(this);
    self->thread_id = target;
    self->mark = poison;
}

switch_to::~switch_to() noexcept
{
    auto* self = for_posix(this);
    assert(self->work == nullptr);
}

bool switch_to::ready() const noexcept
{
    const auto* task = for_posix(this);
    // for background work, always false
    if (task->thread_id == 0) //
        return false;

    // already in the target thread?
    return task->thread_id //
           == internal::current_thread_id();
}

void switch_to::suspend( //
    std::experimental::coroutine_handle<void> coro) noexcept(false)
{
    auto* task = for_posix(this);

    if (task->thread_id != 0)
    {
        // submit to specific thread
        message_t msg{};
        msg.ptr = coro.address();
        post_message(task->thread_id, msg);

        return;
    }

    // submit to background thread. this code will be optimized later
    std::async(std::launch::async,
               [](coroutine_handle<void> frame) -> void {
                   // just continue the work
                   frame.resume();
               },
               coro);
}

void switch_to::resume() noexcept
{
    const auto* task = for_posix(this);

    // check thread id
    if (auto tid = task->thread_id)
        assert(tid == internal::current_thread_id());
}
