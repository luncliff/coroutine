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

section::section(uint16_t) noexcept(false) : storage{}
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

/*
class section_writer : public lockable
{
    reader_writer_lock* rwl;

  public:
    section_writer(reader_writer_lock* lock) : rwl{lock}
    {
    }

  public:
    bool try_lock() noexcept override
    {
        return rwl->try_lock();
    }

    void lock() noexcept(false) override
    {
        return rwl->lock();
    }

    void unlock() noexcept(false) override
    {
        return rwl->unlock();
    }
};
static_assert(sizeof(section_writer) == sizeof(void*) * 2);

class section_reader : public lockable
{
    reader_writer_lock* rwl;

  public:
    section_reader(reader_writer_lock* lock) : rwl{lock}
    {
    }

  public:
    bool try_lock() noexcept override
    {
        return rwl->try_lock_read();
    }

    void lock() noexcept(false) override
    {
        return rwl->lock_read();
    }

    void unlock() noexcept(false) override
    {
        return rwl->unlock();
    }
};
static_assert(sizeof(section_reader) == sizeof(void*) * 2);

lockable& section::reader() noexcept
{
    void* r = storage + 12;
    return *reinterpret_cast<lockable*>(r);
}

lockable& section::writer() noexcept
{
    void* w = storage + 14;
    return *reinterpret_cast<lockable*>(w);
}
*/