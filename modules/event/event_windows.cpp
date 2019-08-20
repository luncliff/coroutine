//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/event.h>
#include <coroutine/frame.h>

#include <gsl/gsl>

#include <system_error>

#include <Windows.h>
#include <synchapi.h>
// Windows Concurrency Runtime's event is not alertible.
//#include <concrt.h>

using namespace std;
using namespace gsl;

namespace coro {

static_assert(is_move_assignable_v<set_or_cancel> == false);
static_assert(is_move_constructible_v<set_or_cancel> == false);
static_assert(is_copy_assignable_v<set_or_cancel> == false);
static_assert(is_copy_constructible_v<set_or_cancel> == false);

GSL_SUPPRESS(con .4)
void __stdcall wait_on_thread_pool(PVOID ctx, BOOLEAN timedout) {
    // we are using INFINITE
    UNREFERENCED_PARAMETER(timedout);
    auto coro = coroutine_handle<void>::from_address(ctx);
    assert(coro.done() == false);
    coro.resume();
}

set_or_cancel::set_or_cancel(HANDLE target_event) noexcept(false)
    : hobject{target_event} {
    // wait object is used as a storage for the event handle
    // until it is going to suspend
}
set_or_cancel::~set_or_cancel() noexcept {
    this->cancel();
}

auto set_or_cancel::cancel() noexcept -> uint32_t {
    // secondary cancel must has no effect
    if (hobject == INVALID_HANDLE_VALUE)
        return NO_ERROR;

    UnregisterWait(hobject);
    hobject = INVALID_HANDLE_VALUE;

    const auto ec = GetLastError();
    if (ec == ERROR_IO_PENDING) {
        // this is expected since we are using INFINITE timeout
        return NO_ERROR;
    }
    return ec;
}

// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-registerwaitforsingleobject
// consider: can we use WT_EXECUTEINWAITTHREAD for this type?
void set_or_cancel::on_suspend(coroutine_handle<void> coro) noexcept(false) {
    // since this point, wo becomes a handle for the request
    // this is one-shot event. so use infinite timeout
    if (RegisterWaitForSingleObject(addressof(hobject), hobject,
                                    wait_on_thread_pool, coro.address(),
                                    INFINITE, WT_EXECUTEONLYONCE) == FALSE) {
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "RegisterWaitForSingleObject"};
    }
}

// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-unregisterwait
uint32_t set_or_cancel::on_resume() noexcept {
    return this->cancel();
}

} // namespace coro
