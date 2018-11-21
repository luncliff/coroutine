// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//    https://weblogs.asp.net/kennykerr/parallel-programming-with-c-part-2-asynchronous-procedure-calls-and-window-messages
//    http://www.wrongbananas.net/cpp/2006.09.13_win32_msg_loops.html
//
// ---------------------------------------------------------------------------

#define NOMINMAX

#include <cassert>
#include <stdexcept>
#include <system_error>

#include <coroutine/switch.h>
#include <coroutine/sync.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <threadpoolapiset.h>

using namespace std::experimental;

bool peek_switched(coroutine_handle<void>& rh) noexcept(false)
{
    message_t msg{};
    if (peek_message(msg) == true)
    {
        rh = coroutine_handle<void>::from_address(msg.ptr);
        return true;
    }
    return false;
}

//// - Note
////      The function won't use GetMessage function
////      since it can disable Overlapped I/O
// bool get(coroutine_handle<void>& rh) noexcept
//{
//    // Handle APC calls (Expecially I/O)
//    // Message >> WAIT_OBJECT_0
//    // Overlapped I/O  >> WAIT_IO_COMPLETION
//    // Failed >> WAIT_FAILED
//    const DWORD reason = ::MsgWaitForMultipleObjectsEx( //
//        0,
//        nullptr, // no handles
//        10'000,  // Not INFINITE.
//        QS_ALLINPUT,
//        MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
//
//    if (reason == WAIT_OBJECT_0) // Message received
//        return peek(rh);
//
//    // it might be one of Handled I/O || Timeout || Error
//    // just return false.
//    return false;
//}

static_assert(std::is_nothrow_move_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_move_constructible_v<switch_to> == false);
static_assert(std::is_nothrow_copy_assignable_v<switch_to> == false);
static_assert(std::is_nothrow_copy_constructible_v<switch_to> == false);

// - Note
//      Thread Pool Callback. Expect `noexcept` operation
void WINAPI resume_coroutine( //
    PTP_CALLBACK_INSTANCE instance,
    PVOID context,
    PTP_WORK work) noexcept
{
    // debug info from these args?
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(work);

    coroutine_handle<void>::from_address(context).resume();
}

struct switch_to_win32
{
    DWORD thread_id{};
    DWORD mark{};
    PTP_WORK work{};
};
static_assert(sizeof(switch_to_win32) <= sizeof(switch_to));

switch_to::switch_to(uint32_t target) noexcept : u64{}
{
    constexpr uint32_t poison =
        std::numeric_limits<uint32_t>::max() - 0xFADE'BCFA;

    auto* self = reinterpret_cast<switch_to_win32*>(this);
    self->thread_id = target;
    self->mark = poison;
}

switch_to::~switch_to() noexcept
{
    auto* self = reinterpret_cast<const switch_to_win32*>(this);
    const auto work = self->work;

    // Windows Thread Pool's Work Item
    if (work != PTP_WORK{}) ::CloseThreadpoolWork(work);
}

bool switch_to::ready() const noexcept
{
    auto* self = reinterpret_cast<const switch_to_win32*>(this);

    // non-zero : Specific thread's queue
    //     zero : Windows Thread Pool
    if (auto tid = self->thread_id)
        // Switch to another thread_id?
        return tid == ::GetCurrentThreadId();

    // Switch to a specific thread
    return false;
}

//  - Caution
//
//  The caller will suspend after return.
//  So this is the last chance to notify the error
//
//  If the target thread can't receive message, there is nothing the code can
//  do.
//
//  Since there is no way to create a thread with a designated ID,
//  this function will throw exception. (which might kill the program)
//  The SERIOUS error situation must be prevented before this code.
//
void switch_to::suspend(coroutine_handle<void> rh) noexcept(false)
{
    auto* self = reinterpret_cast<switch_to_win32*>(this);
    // non-zero : Specific thread's queue
    //     zero : Windows Thread Pool
    const auto thread_id = self->thread_id;

    // Submit work to Windows Thread Pool API
    if (thread_id == 0)
    {
        auto& pwk = self->work;

        // work allocation
        if (pwk == PTP_WORK{})
        {
            pwk =
                ::CreateThreadpoolWork(resume_coroutine, rh.address(), nullptr);

            // allocation failed
            if (pwk == PTP_WORK{})
                throw std::system_error{static_cast<int>(GetLastError()),
                                        std::system_category()};
        }

        ::SubmitThreadpoolWork(pwk);
    }
    // Post the work to the given thread
    else
    {
        message_t msg{};
        msg.ptr = rh.address();

        post_message(thread_id, msg);
    }
    return;
}

void switch_to::resume() noexcept
{
#ifdef _DEBUG
    auto* self = reinterpret_cast<const switch_to_win32*>(this);

    if (auto tid = self->thread_id) // Are we in correct thread_id?
        assert(tid == ::GetCurrentThreadId());
#endif
}
