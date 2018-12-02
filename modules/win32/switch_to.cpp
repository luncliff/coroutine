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

#include <cassert>
#include <system_error>

// #include <gsl/gsl_util>

#define WIN32_LEAN_AND_MEAN
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

//// - Note
////      Thread Pool Callback. Expect `noexcept` operation
// void resume_coroutine(PTP_CALLBACK_INSTANCE instance,
//                      PVOID context,
//                      PTP_WORK work) noexcept
//{
//    // debug info from these args?
//    UNREFERENCED_PARAMETER(instance);
//    UNREFERENCED_PARAMETER(work);
//
//    coroutine_handle<void>::from_address(context).resume();
//}

struct switch_to_win32
{
    thread_id_t tid{};
    PTP_WORK work{};
    coroutine_handle<void> coro{};
};
static_assert(sizeof(switch_to_win32) <= sizeof(switch_to));

// GSL_SUPPRESS(type.1)
auto for_win32(switch_to* s) noexcept
{
    return reinterpret_cast<switch_to_win32*>(s);
}

// GSL_SUPPRESS(type.1)
auto for_win32(const switch_to* s) noexcept
{
    return reinterpret_cast<const switch_to_win32*>(s);
}

void CALLBACK _activate_( // resume switched tasks
    PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK) noexcept(false)
{
    auto* sw = static_cast<switch_to_win32*>(context);

#ifdef _DEBUG
    // check again for safety
    assert(sw->coro.done() == false);
#endif

    sw->coro.resume();
}

switch_to::switch_to(uint64_t target) noexcept(false) : storage{}
{
    auto* sw = for_win32(this);
    if (target != 0)
    {
        sw->tid = static_cast<thread_id_t>(target);
        return;
    }

    sw->work = CreateThreadpoolWork(_activate_, this, nullptr);
    // throw if allocation failed
    if (sw->work == nullptr)
    {
        auto ec = static_cast<int>(GetLastError());
        throw std::system_error{ec, std::system_category(),
                                "CreateThreadpoolWork"};
    }
}

switch_to::~switch_to() noexcept
{
    auto* sw = for_win32(this);
    if (sw->work)
        ::CloseThreadpoolWork(sw->work);

    storage[0] = 0;
}

bool switch_to::ready() const noexcept
{
    auto* sw = for_win32(this);

    // non-zero : Specific thread's queue
    //     zero : Windows Thread Pool
    if (sw->tid != thread_id_t{})
        // already in the thread ?
        return sw->tid == current_thread_id();

    // trigger switching
    return false;
}

//  - Caution
//
//  The caller will suspend after return.
//  So this is the last chance to notify the error
//
//  If the target thread can't receive message,
//  there is nothing this code can do.
//
//  Since there is no way to create a thread with a designated ID,
//  this function will throw exception. (which might kill the program)
//  The SERIOUS error situation must be prevented before this code.
//
void switch_to::suspend(coroutine_handle<void> rh) noexcept(false)
{
    auto* sw = for_win32(this);
    // Post work to the thread
    if (sw->tid != thread_id_t{})
    {
        message_t msg{};
        msg.ptr = rh.address();

        // ... possible throw here ...
        post_message(sw->tid, msg);
    }
    // Submit work to Windows Thread Pool API
    else
    {
        sw->coro = rh;
        ::SubmitThreadpoolWork(sw->work);
    }
}

void switch_to::resume() noexcept
{
#ifdef _DEBUG
    const auto* sw = for_win32(this);
    if (sw->tid != thread_id_t{})
        // Are we in correct thread_id?
        assert(sw->tid == current_thread_id());
#endif
}
