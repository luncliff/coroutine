// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <atomic>
#include <system_error>

#include <Windows.h>

#include <gsl/gsl>

using namespace std;

struct wait_group_win32 final
{
    HANDLE ev{};
    std::atomic<uint32_t> count{};
    SECURITY_ATTRIBUTES attr{};
};

GSL_SUPPRESS(type .1)
auto for_win32(wait_group* wg) noexcept -> gsl::not_null<wait_group_win32*>
{
    static_assert(sizeof(wait_group_win32) <= sizeof(wait_group));
    return reinterpret_cast<wait_group_win32*>(wg);
}

GSL_SUPPRESS(r .11)
wait_group::wait_group() noexcept(false) : storage{}
{
    auto wg = for_win32(this);
    new (wg) wait_group_win32{};

    wg->ev = CreateEventA(nullptr, false, false, nullptr);

    if (wg->ev == INVALID_HANDLE_VALUE)
        throw system_error{gsl::narrow_cast<int>(GetLastError()),
                           system_category(), "CreateEventA"};
}

GSL_SUPPRESS(con .4)
wait_group::~wait_group() noexcept
{
    auto wg = for_win32(this);

    if (wg->ev != INVALID_HANDLE_VALUE)
    {
        // close and leave
        CloseHandle(wg->ev);
        wg->ev = INVALID_HANDLE_VALUE;
    }
}

GSL_SUPPRESS(con .4)
void wait_group::add(uint16_t delta) noexcept
{
    auto wg = for_win32(this);
    wg->count.fetch_add(delta, memory_order::memory_order_acq_rel);
}

GSL_SUPPRESS(con .4)
void wait_group::done() noexcept
{
    auto wg = for_win32(this);
    wg->count.fetch_sub(1, memory_order::memory_order_acq_rel);

    // load again without reordering
    if (wg->count.load(memory_order::memory_order_acquire) == 0)
        // set event
        SetEvent(wg->ev);
}

GSL_SUPPRESS(con .4)
bool wait_group::wait(duration d) noexcept(false)
{
    auto wg = for_win32(this);

    while (wg->count > 0)
    {
        // This makes APC available.
        // expecially for Overlapped I/O
        DWORD ec = WaitForSingleObjectEx(
            wg->ev, gsl::narrow_cast<DWORD>(d.count()), TRUE);

        if (ec == WAIT_FAILED)
            // update error code
            ec = GetLastError();

        // the only case for success
        else if (ec == WAIT_OBJECT_0)
            break;

        // timeout. this is expected failure
        // the user code can try again
        else if (ec == WAIT_TIMEOUT)
            return false;

        // return because of APC. same with WAIT_TIMEOUT
        else if (ec == WAIT_IO_COMPLETION)
            return false;

        // ... the other case will be considered exception ...
        throw system_error{gsl::narrow_cast<int>(ec), system_category(),
                           "WaitForSingleObjectEx"};
    }

    this->~wait_group();
    return true;
}
