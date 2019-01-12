// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/frame.h>
#include <coroutine/switch.h>

#include <cassert>
#include <csignal>
#include <future>
#include <system_error>

using namespace std;
using namespace std::experimental;

static_assert(std::is_nothrow_move_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_move_constructible_v<switch_to> == false);
static_assert(std::is_nothrow_copy_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_copy_constructible_v<switch_to> == false);

struct switch_to_posix final
{
    atomic_bool is_closed{};
    std::unique_ptr<messaging_queue_t> queue{};
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

switch_to::switch_to() noexcept(false) : storage{}
{
    auto* sw = for_posix(this);

    new (sw) switch_to_posix{};
    sw->is_closed = false;
    sw->queue = create_message_queue();
}

switch_to::~switch_to() noexcept
{
    auto* sw = for_posix(this);
    auto trunc = std::move(sw->queue);
}

bool switch_to::ready() const noexcept
{
    return false; // always trigger switching
}

void switch_to::suspend(
    std::experimental::coroutine_handle<void> coro) noexcept(false)
{
    auto* sw = for_posix(this);
    message_t msg{};
    msg.ptr = coro.address();

    // expect consumer will pop from the queue fast enough
    while (sw->queue->post(msg) == false)
        std::this_thread::yield();
}

void switch_to::resume() noexcept
{
    // nothing to do
}

auto* for_posix(const scheduler_t* s) noexcept
{
    static_assert(sizeof(switch_to_posix) <= sizeof(scheduler_t));
    return reinterpret_cast<const switch_to_posix*>(s);
}
auto* for_posix(scheduler_t* s) noexcept
{
    static_assert(sizeof(switch_to_posix) <= sizeof(scheduler_t));
    return reinterpret_cast<switch_to_posix*>(s);
}

auto switch_to::scheduler() noexcept(false) -> scheduler_t&
{
    static_assert(sizeof(scheduler_t) == sizeof(switch_to));
    return *(reinterpret_cast<scheduler_t*>(this));
}

void scheduler_t::close() noexcept
{
    auto* sw = for_posix(this);
    sw->is_closed = true;
}

bool scheduler_t::closed() const noexcept
{
    auto* sw = for_posix(this);
    return sw->is_closed;
}

auto scheduler_t::wait(duration d) noexcept(false) -> coroutine_task
{
    auto* sw = for_posix(this);
    message_t m{};

    // discarding boolean return
    sw->queue->wait(m, d);
    return coroutine_task::from_address(m.ptr);
}
