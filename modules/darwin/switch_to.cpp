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

using namespace std;
using namespace std::experimental;

bool peek_switched( // ... better name?
    std::experimental::coroutine_handle<void>& coro) noexcept(false)
{
    message_t msg{};
    if (peek_message(msg) == true)
    {
        coro = coroutine_handle<void>::from_address(msg.ptr);
        // ensure again
        // because sender might return empty frame(null)
        return msg.ptr != nullptr;
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

auto* for_posix(switch_to* s) noexcept
{
    static_assert(sizeof(switch_to_posix) <= sizeof(switch_to));
    return reinterpret_cast<switch_to_posix*>(s);
}
auto* for_posix(const switch_to* s) noexcept
{
    static_assert(sizeof(switch_to_posix) <= sizeof(switch_to));
    return reinterpret_cast<const switch_to_posix*>(s);
}

switch_to::switch_to(thread_id_t target) noexcept(false) : storage{}
{
    auto* sw = for_posix(this);

    new (sw) switch_to_posix{};
    sw->thread_id = target;
}

switch_to::~switch_to() noexcept
{
    auto* sw = for_posix(this);
    assert(sw->work == nullptr);
}

bool switch_to::ready() const noexcept
{
    const auto* sw = for_posix(this);

    if (sw->thread_id != thread_id_t{})
        // check if already in the target thread
        return sw->thread_id == current_thread_id();

    // for background work, always false
    return false;
}

void post_to_background(
    std::experimental::coroutine_handle<void> coro) noexcept(false);

void switch_to::suspend(
    std::experimental::coroutine_handle<void> coro) noexcept(false)
{
    auto* sw = for_posix(this);

    if (sw->thread_id == thread_id_t{})
        return post_to_background(coro);

    // submit to specific thread
    message_t msg{};
    msg.ptr = coro.address();

    if (post_message(sw->thread_id, msg) == false)
        throw std::runtime_error{"post_message failed in suspend"};
}

void switch_to::resume() noexcept
{
    const auto* sw = for_posix(this);

    if (sw->thread_id != thread_id_t{})
        // check thread id
        assert(sw->thread_id == current_thread_id());
}
