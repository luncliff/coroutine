// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      This file is distributed under Creative Commons 4.0-BY License
//
// ---------------------------------------------------------------------------
#include <magic/switch.h>

#include <cassert>
#include <system_error>
#include <stdexcept>

namespace magic
{

// Actually, this is application-defined message code
constexpr auto WM_SWITCH = WM_USER + 0x06AF;

bool peek(stdex::coroutine_handle<> &rh) noexcept
{
    MSG msg{};
    // Peek from thread's message queue
    if (PeekMessageW(&msg, static_cast<HWND>(INVALID_HANDLE_VALUE),
                     WM_SWITCH, WM_SWITCH, PM_REMOVE))
    {
        PVOID ptr = reinterpret_cast<PVOID>(msg.wParam);
        rh = stdex::coroutine_handle<>::from_address(ptr);
        return true;
    }
    return false;
}

// - Note
//      The function won't use GetMessage function
//      since it can disable Overlapped I/O
// - See Also
//      https://weblogs.asp.net/kennykerr/parallel-programming-with-c-part-2-asynchronous-procedure-calls-and-window-messages
//      http://www.wrongbananas.net/cpp/2006.09.13_win32_msg_loops.html
bool get(stdex::coroutine_handle<> &rh) noexcept
{
    // Handle APC calls (Expecially I/O)
    const DWORD reason = ::MsgWaitForMultipleObjectsEx(
        0, nullptr, // no handles
        10'000,     // INFINITE
        QS_ALLINPUT,
        MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

    //assert(
    //    reason == WAIT_OBJECT_0 ||  // Message ?
    //    reason == WAIT_IO_COMPLETION || // Overlapped I/O ?
    //    reason == WAIT_FAILED // Failed?
    //);

    if (reason == WAIT_OBJECT_0) // Message received
        return peek(rh);

    // Handled I/O || Error
    //      return anyway...
    return false;
    //return GetLastError();
}

switch_to::switch_to(uint32_t target) noexcept : thread{target}, work{}
{
    // ...
}

switch_to::switch_to(switch_to && rhs) noexcept
{
    std::swap(this->thread, rhs.thread);
    std::swap(this->work, rhs.work);
}

switch_to & switch_to::operator=(switch_to && rhs) noexcept
{
    std::swap(this->thread, rhs.thread);
    std::swap(this->work, rhs.work);
    return *this;
}

switch_to::~switch_to() noexcept
{
    if (work != PTP_WORK{})
        ::CloseThreadpoolWork(work);
}

bool switch_to::ready() const noexcept
{
    if (thread)
        // Switch to another thread?
        return thread == ::GetCurrentThreadId();
    else
        // Switch to thread pool
        return false;
}


//  - Caution
//
//  The caller is going to suspend after return.
//  So this block is the last chance to notify the error
//  if the target thread can't receive rh(resumeable handle).
//
//  Since there is no way to create a thread with a designated ID, 
//  this function will throw exception to kill the program.
//  The SERIOUS error situation must be prevented before this code.
//
//  To ensure warning about this function, it uses throw specification mismatch.
//
#pragma warning ( disable : 4297 )
void switch_to::suspend(stdex::coroutine_handle<> rh) noexcept
{
    static_assert(sizeof(WPARAM) == sizeof(stdex::coroutine_handle<>),
                  "WPARAM in not enough to hold data");

    // Submit work to Windows thread pool
    if (this->thread == 0)
    {
        this->work = ::CreateThreadpoolWork(onWork, rh.address(), nullptr);
        // Thread work allocation failure
        if (this->work == PTP_WORK{})
            goto OnWinAPIError;

        ::SubmitThreadpoolWork(this->work);
    }
    // Post the work to the thread
    else
    {
        // WParam holds handle's address
        const WPARAM wp = reinterpret_cast<WPARAM>(rh.address());
        const BOOL posted = PostThreadMessageW(this->thread, WM_SWITCH, wp, LPARAM{});
        // !!! Read Caution !!!
        if (posted == FALSE)
            goto OnWinAPIError;
    }

    return;

OnWinAPIError:
    const auto ec = std::error_code{ 
        static_cast<int>(GetLastError()), std::system_category() };
#ifdef _DEBUG
    fputs(ec.message().c_str(), stderr);
#endif
    throw ec;
}

void switch_to::resume() noexcept
{
#ifdef _DEBUG
    // Are we in correct thread?
    if (thread)
        assert(thread == ::GetCurrentThreadId());
#endif
}

void switch_to::onWork(PTP_CALLBACK_INSTANCE, PVOID pContext, PTP_WORK) noexcept
{
    return stdex::coroutine_handle<>::from_address(pContext).resume();
}

} // magic
