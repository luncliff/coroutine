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

#include <gsl/gsl>

#include <concrt.h> // Windows Concurrency Runtime
using namespace concurrency;

GSL_SUPPRESS(type .1)
auto for_win32(section* s) noexcept -> gsl::not_null<reader_writer_lock*>
{
    static_assert(sizeof(reader_writer_lock) <= sizeof(section));
    return reinterpret_cast<reader_writer_lock*>(s);
}

GSL_SUPPRESS(r .11)
section::section() noexcept(false) : storage{}
{
    new (for_win32(this)) reader_writer_lock{};
}

GSL_SUPPRESS(f .6)
GSL_SUPPRESS(f .11)
section::~section() noexcept
{
    try
    {
        for_win32(this)->~reader_writer_lock();
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
    }
}

GSL_SUPPRESS(f .6)
bool section::try_lock() noexcept
{
    return for_win32(this)->try_lock();
}

void section::lock() noexcept(false)
{
    return for_win32(this)->lock();
}

void section::unlock() noexcept(false)
{
    return for_win32(this)->unlock();
}
