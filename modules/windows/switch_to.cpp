// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//    https://weblogs.asp.net/kennykerr/parallel-programming-with-c-part-2-asynchronous-procedure-calls-and-window-messages
//    http://www.wrongbananas.net/cpp/2006.09.13_win32_msg_loops.html
//
// ---------------------------------------------------------------------------
#include <coroutine/frame.h>
#include <coroutine/switch.h>

#include <gsl/gsl>

#include <atomic>

using namespace std;
using namespace std::experimental;

static_assert(std::is_nothrow_move_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_move_constructible_v<switch_to> == false);
static_assert(std::is_nothrow_copy_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_copy_constructible_v<switch_to> == false);

struct switch_to_win32 final
{
    std::atomic_bool is_closed{};
    std::unique_ptr<messaging_queue_t> queue{};
};

GSL_SUPPRESS(type .1)
auto for_win32(switch_to* s) noexcept -> gsl::not_null<switch_to_win32*>
{
    static_assert(sizeof(switch_to_win32) <= sizeof(switch_to));
    return reinterpret_cast<switch_to_win32*>(s);
}

GSL_SUPPRESS(type .1)
auto for_win32(const switch_to* s) noexcept
    -> gsl::not_null<const switch_to_win32*>
{
    static_assert(sizeof(switch_to_win32) <= sizeof(switch_to));
    return reinterpret_cast<const switch_to_win32*>(s);
}

GSL_SUPPRESS(r .11)
switch_to::switch_to() noexcept(false) : storage{}
{
    auto sw = for_win32(this);

    new (sw) switch_to_win32{};
    sw->is_closed = false;
    sw->queue = create_message_queue();
}

GSL_SUPPRESS(con .4)
switch_to::~switch_to() noexcept
{
    auto sw = for_win32(this);
    auto trunc = std::move(sw->queue);
}

bool switch_to::ready() const noexcept
{
    return false; // always trigger switching
}

GSL_SUPPRESS(con .4)
void switch_to::suspend(
    std::experimental::coroutine_handle<void> coro) noexcept(false)
{
    auto sw = for_win32(this);
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

GSL_SUPPRESS(type .1)
auto for_win32(const scheduler_t* s) noexcept
    -> gsl::not_null<const switch_to_win32*>
{
    static_assert(sizeof(switch_to_win32) <= sizeof(scheduler_t));
    return reinterpret_cast<const switch_to_win32*>(s);
}

GSL_SUPPRESS(type .1)
auto for_win32(scheduler_t* s) noexcept -> gsl::not_null<switch_to_win32*>
{
    static_assert(sizeof(switch_to_win32) <= sizeof(scheduler_t));
    return reinterpret_cast<switch_to_win32*>(s);
}

auto switch_to::scheduler() noexcept(false) -> scheduler_t&
{
    static_assert(sizeof(scheduler_t) == sizeof(switch_to));
    return *(reinterpret_cast<scheduler_t*>(this));
}

GSL_SUPPRESS(con .4)
void scheduler_t::close() noexcept
{
    auto sw = for_win32(this);
    sw->is_closed = true;
}

GSL_SUPPRESS(con .4)
bool scheduler_t::closed() const noexcept
{
    auto sw = for_win32(this);
    return sw->is_closed;
}

GSL_SUPPRESS(con .4)
auto scheduler_t::wait(duration d) noexcept(false) -> coroutine_task
{
    auto sw = for_win32(this);
    message_t m{};

    // discarding boolean return
    sw->queue->wait(m, d);
    return coroutine_task::from_address(m.ptr);
}
