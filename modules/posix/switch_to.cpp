// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/switch.h>
#include <coroutine/sync.h>

#include <cassert>
#include <stdexcept>
#include <system_error>

using namespace std;
using namespace std::experimental;

bool peek_switched(coroutine_handle<void>&) noexcept(false)
{
    throw std::exception{};
    // const auto tid = GetCurrentThreadId();
    // message_t msg{};
    // if (peek_message(tid, msg) == true)
    // {
    //     rh = coroutine_handle<void>::from_address(msg.ptr);
    //     return true;
    // }
    return false;
}

static_assert(std::is_nothrow_move_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_move_constructible_v<switch_to> == false);
static_assert(std::is_nothrow_copy_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_copy_constructible_v<switch_to> == false);

// // - Note
// //      Thread Pool Callback. Expect `noexcept` operation
// void resume_coroutine(PTP_CALLBACK_INSTANCE instance,
//                       PVOID context,
//                       PTP_WORK work) noexcept
// {
//     // debug info from these args?
//     UNREFERENCED_PARAMETER(instance);
//     UNREFERENCED_PARAMETER(work);
//     coroutine_handle<void>::from_address(context).resume();
// }

struct switch_to_posix
{
    uint32_t thread_id{};
    uint32_t mark{};
    void* work{};
};
static_assert(sizeof(switch_to) == sizeof(switch_to_posix));

auto& reinterpret(switch_to* s) noexcept
{
    return *reinterpret_cast<switch_to_posix*>(s);
}
auto& reinterpret(const switch_to* s) noexcept
{
    return *reinterpret_cast<const switch_to_posix*>(s);
}

switch_to::switch_to(uint32_t target) noexcept : u64{}
{
    constexpr uint32_t poison =
        std::numeric_limits<uint32_t>::max() - 0xFADE'BCFA;

    auto& self = reinterpret(this);
    self.thread_id = target;
    self.mark = poison;
}

switch_to::~switch_to() noexcept
{
    auto& self = reinterpret(this);
    assert(self.work == nullptr);
}

bool switch_to::ready() const noexcept
{
    auto& self = reinterpret(this);
    return self.work == nullptr; // just true
}

void switch_to::suspend(coroutine_handle<void>) noexcept(false)
{
    throw std::exception{};
}

void switch_to::resume() noexcept
{
    auto& self = reinterpret(this);
    // do nothing ...
}
