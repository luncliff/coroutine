// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------
#define NOMINMAX

#include <coroutine/sync.h>
#include <system_error>

#include <Windows.h> // System API

wait_group::wait_group() noexcept(false) : event{INVALID_HANDLE_VALUE}, count{0}
{
    event = CreateEvent(nullptr,  // default security attributes
                        false,    // auto-reset event object
                        false,    // initial state is nonsignaled
                        nullptr); // unnamed object

    if (event == INVALID_HANDLE_VALUE)
        throw std::system_error{static_cast<int>(GetLastError()),
                                std::system_category()};
}

wait_group::~wait_group() noexcept
{
    if (event != INVALID_HANDLE_VALUE) CloseHandle(event);
}

void wait_group::add(uint32_t delta) noexcept { this->count.fetch_add(delta); }

void wait_group::done() noexcept
{
    if (event == INVALID_HANDLE_VALUE) return;

    if (count > 0) count.fetch_sub(1);

    if (count == 0) SetEvent(event);
}

void wait_group::wait(uint32_t timeout) noexcept(false)
{
    static_assert(sizeof(uint32_t) == sizeof(DWORD), "Invalid timeout type");

    while (count > 0)
    {
        // This makes APC available.
        // expecially for Overlapped I/O
        auto reason = WaitForSingleObjectEx(event, timeout, TRUE);

        // return because of APC. continue the wait
        if (reason == WAIT_IO_COMPLETION) continue;

        // the only case for success
        if (reason == WAIT_OBJECT_0)
        {
            CloseHandle(event);
            event = INVALID_HANDLE_VALUE;
            return;
        }

        // ... the other case will be considered exception ...
        throw std::system_error{static_cast<int>(GetLastError()),
                                std::system_category()};
    }
    return;
}
