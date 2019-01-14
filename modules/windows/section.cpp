// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Reference
//      - CRITICAL_SECTION
//      - SRWLock
//      -
//      https://docs.microsoft.com/en-us/cpp/parallel/concrt/concurrency-runtime?view=vs-2017
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
// #include <gsl/gsl_assert>

#include <concrt.h> // Windows Concurrency Runtime
using namespace concurrency;

auto for_win32(section* s) noexcept
{
    static_assert(sizeof(reader_writer_lock) <= sizeof(section));
    return reinterpret_cast<reader_writer_lock*>(s);
}

section::section() noexcept(false) : storage{}
{
    auto rwl = for_win32(this);
    new (rwl) reader_writer_lock{};
}

section::~section() noexcept
{
    auto rwl = for_win32(this);
    try
    {
        rwl->~reader_writer_lock();
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
    }
}

// GSL_SUPPRESS(f.6)
bool section::try_lock() noexcept
{
    auto rwl = for_win32(this);
    return rwl->try_lock();
}

void section::lock() noexcept(false)
{
    auto rwl = for_win32(this);
    return rwl->lock();
}

void section::unlock() noexcept(false)
{
    auto rwl = for_win32(this);
    return rwl->unlock();
}
