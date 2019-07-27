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

static_assert(is_move_assignable_v<ptp_event> == false);
static_assert(is_move_constructible_v<ptp_event> == false);
static_assert(is_copy_assignable_v<ptp_event> == false);
static_assert(is_copy_constructible_v<ptp_event> == false);

GSL_SUPPRESS(con .4)
void __stdcall ptp_event::wait_on_thread_pool(PVOID ctx, BOOLEAN timedout) {
    // we are using INFINITE
    UNREFERENCED_PARAMETER(timedout);
    auto coro = coroutine_handle<void>::from_address(ctx);
    assert(coro.done() == false);
    coro.resume();
}

ptp_event::ptp_event(HANDLE ev) noexcept(false) : wo{ev} {
    // wait object is used as a storage for the event handle
    // until it is going to suspend
}
ptp_event::~ptp_event() noexcept {
    if (wo != INVALID_HANDLE_VALUE)
        this->cancel();
}
auto ptp_event::cancel() noexcept -> uint32_t {
    if (UnregisterWait(wo))
        wo = INVALID_HANDLE_VALUE;

    const auto ec = GetLastError();
    if (ec == ERROR_IO_PENDING)
        // this is expected since we are using INFINITE timeout
        return NO_ERROR;
    return ec;
}

// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-registerwaitforsingleobject
// consider: can we use WT_EXECUTEINWAITTHREAD for this type?
void ptp_event::on_suspend(coroutine_handle<void> coro) noexcept(false) {
    // since this point, wo becomes a handle for the request
    auto event_object = wo;
    // this is one-shot event. so use infinite timeout
    if (RegisterWaitForSingleObject(addressof(wo), event_object,
                                    wait_on_thread_pool, coro.address(),
                                    INFINITE, WT_EXECUTEONLYONCE) == FALSE) {
        const auto ec = gsl::narrow_cast<int>(GetLastError());
        throw system_error{ec, system_category(),
                           "RegisterWaitForSingleObject"};
    }
}

// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-unregisterwait
uint32_t ptp_event::on_resume() noexcept {
    if (wo == INVALID_HANDLE_VALUE) // already canceled
        return NO_ERROR;
    return this->cancel();
}

} // namespace coro