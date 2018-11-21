// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOCRYPT
#include <Windows.h>

// static_assert(sizeof(section) == SYSTEM_CACHE_ALIGNMENT_SIZE);
static_assert(sizeof(CRITICAL_SECTION) <= sizeof(section));

section::section(uint16_t spin) noexcept
{
    auto* section = reinterpret_cast<CRITICAL_SECTION*>(this->u64);
    // https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-initializecriticalsectionandspincount
    ::InitializeCriticalSectionAndSpinCount(section, spin);
}

section::~section() noexcept
{
    auto* section = reinterpret_cast<CRITICAL_SECTION*>(this->u64);
    ::DeleteCriticalSection(section);
}

bool section::try_lock() noexcept
{
    auto* section = reinterpret_cast<CRITICAL_SECTION*>(this->u64);
    return ::TryEnterCriticalSection(section) == TRUE;
}

void section::lock() noexcept
{
    auto* section = reinterpret_cast<CRITICAL_SECTION*>(this->u64);
    return ::EnterCriticalSection(section);
}

void section::unlock() noexcept
{
    auto* section = reinterpret_cast<CRITICAL_SECTION*>(this->u64);
    return ::LeaveCriticalSection(section);
}

// https://docs.microsoft.com/en-us/windows/desktop/Sync/slim-reader-writer--srw--locks
static_assert(sizeof(RTL_SRWLOCK) <= sizeof(exclusive_slim));

exclusive_slim::exclusive_slim() noexcept
{
    auto* srw = reinterpret_cast<RTL_SRWLOCK*>(this);
    InitializeSRWLock(srw);
}

exclusive_slim::~exclusive_slim() noexcept
{
    // do nothing since slim read/write lock
    //  doesn't need to be destroyed
}

bool exclusive_slim::try_lock() noexcept
{
    auto* srw = reinterpret_cast<RTL_SRWLOCK*>(this);
    return TryAcquireSRWLockExclusive(srw) == TRUE;
}
void exclusive_slim::lock() noexcept
{
    auto* srw = reinterpret_cast<RTL_SRWLOCK*>(this);
    AcquireSRWLockExclusive(srw);
}
void exclusive_slim::unlock() noexcept
{
    auto* srw = reinterpret_cast<RTL_SRWLOCK*>(this);
    ReleaseSRWLockExclusive(srw);
}