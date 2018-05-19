// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <magic/switch.h>

#include <cassert>
#include <stdexcept>
#include <system_error>

namespace magic {

// Actually, this is application-defined message code
constexpr auto WM_SWITCH = WM_USER + 0x06AF;

bool peek(stdex::coroutine_handle<> &rh) noexcept {
  MSG msg{};
  // Peek from thread's message queue
  if (PeekMessageW(&msg, static_cast<HWND>(INVALID_HANDLE_VALUE), WM_SWITCH,
                   WM_SWITCH, PM_REMOVE)) {
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
bool get(stdex::coroutine_handle<> &rh) noexcept {
  // Handle APC calls (Expecially I/O)
  // Message >> WAIT_OBJECT_0
  // Overlapped I/O  >> WAIT_IO_COMPLETION
  // Failed >> WAIT_FAILED
  const DWORD reason = ::MsgWaitForMultipleObjectsEx(
      0, nullptr, // no handles
      10'000,     // Not INFINITE.
      QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

  if (reason == WAIT_OBJECT_0) // Message received
    return peek(rh);

  // it might be one of Handled I/O || Timeout || Error
  // just return false.
  return false;
}

switch_to::switch_to(uint32_t target) noexcept : thread{target}, work{} {
  // ...
}

switch_to::switch_to(switch_to &&rhs) noexcept {
  std::swap(this->thread, rhs.thread);
  std::swap(this->work, rhs.work);
}

switch_to &switch_to::operator=(switch_to &&rhs) noexcept {
  std::swap(this->thread, rhs.thread);
  std::swap(this->work, rhs.work);
  return *this;
}

switch_to::~switch_to() noexcept {
  if (this->work != PTP_WORK{})
    ::CloseThreadpoolWork(this->work);
}

bool switch_to::ready() const noexcept {
  if (this->thread)
    // Switch to another thread?
    return this->thread == ::GetCurrentThreadId();
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
//  this function will throw exception. (which might kill the program)
//  The SERIOUS error situation must be prevented before this code.
//
//  To ensure warning about this function, it uses throw specification mismatch.
//
void switch_to::suspend(stdex::coroutine_handle<> rh) noexcept(false) {
  static_assert(sizeof(WPARAM) == sizeof(stdex::coroutine_handle<>),
                "WPARAM in not enough to hold data");

  // Submit work to Windows thread pool
  if (this->thread == 0) {
    this->work = ::CreateThreadpoolWork(onWork, rh.address(), nullptr);
    // Thread work allocation failure
    if (this->work == PTP_WORK{})
      goto OnWinAPIError;

    ::SubmitThreadpoolWork(this->work);
  }
  // Post the work to the thread
  else {
    // WParam holds handle's address
    const WPARAM wp = reinterpret_cast<WPARAM>(rh.address());
    const BOOL posted =
        ::PostThreadMessageW(this->thread, WM_SWITCH, wp, LPARAM{});
    // !!! Read Caution !!!
    if (posted == FALSE)
      goto OnWinAPIError;
  }

  return;

OnWinAPIError:
  const auto eval = static_cast<int>(::GetLastError());
  const auto &&emsg = std::system_category().message(eval);

  throw std::runtime_error{emsg};
}

void switch_to::resume() noexcept {
#ifdef _DEBUG
  // Are we in correct thread?
  if (this->thread)
    assert(this->thread == ::GetCurrentThreadId());
#endif
}

void switch_to::onWork(PTP_CALLBACK_INSTANCE, PVOID pContext,
                       PTP_WORK) noexcept {
  return stdex::coroutine_handle<>::from_address(pContext).resume();
}

} // namespace magic
