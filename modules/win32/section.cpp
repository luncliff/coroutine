// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

#define NOMINMAX

#include <coroutine/sync.h>
#include <system_error>

#include <Windows.h> // System API

// static_assert(sizeof(section) == SYSTEM_CACHE_ALIGNMENT_SIZE);
// static_assert(sizeof(CRITICAL_SECTION) <= sizeof(section));

constexpr auto reinterpret(section& s) noexcept
{
    return reinterpret_cast<CRITICAL_SECTION*>(std::addressof(s));
}

section::section(uint16_t spin) noexcept
{
    auto* section = reinterpret(*this);
    // https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-initializecriticalsectionandspincount
    ::InitializeCriticalSectionAndSpinCount(section, spin);
}

section::~section() noexcept
{
    auto* section = reinterpret(*this);
    ::DeleteCriticalSection(section);
}

bool section::try_lock() noexcept
{
    auto* section = reinterpret(*this);
    return ::TryEnterCriticalSection(section) == TRUE;
}

void section::lock() noexcept
{
    auto* section = reinterpret(*this);
    return ::EnterCriticalSection(section);
}

void section::unlock() noexcept
{
    auto* section = reinterpret(*this);
    return ::LeaveCriticalSection(section);
}
