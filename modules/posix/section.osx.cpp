// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>
#include <system_error>

#include <pthread.h>   // implement over pthread API
#include <sys/types.h> // system types

struct section_osx final
{
    pthread_mutex_t mtx;
};

auto for_osx(section* s) noexcept
{
    static_assert(sizeof(section_osx) <= sizeof(section));
    return reinterpret_cast<section_osx*>(s);
}
auto for_osx(const section* s) noexcept
{
    static_assert(sizeof(section_osx) <= sizeof(section));
    return reinterpret_cast<const section_osx*>(s);
}

section::section(uint16_t) noexcept(false) : storage{}
{
    int ec = 0;
    const char* message = nullptr;
    pthread_mutexattr_t attr{};

    auto* osx = for_osx(this);
    auto* mtx = std::addressof(osx->mtx);

    ec = pthread_mutexattr_init(&attr);
    if (ec)
    {
        message = "pthread_mutexattr_init";
        goto OnSystemError;
    }
    ec = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (ec)
    {
        message = "pthread_mutexattr_settype";
        goto OnSystemError;
    }
    ec = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);
    if (ec)
    {
        message = "pthread_mutexattr_setprotocol";
        goto OnSystemError;
    }
    ec = pthread_mutex_init(mtx, &attr);
    if (ec)
    {
        message = "pthread_mutex_init";
        goto OnSystemError;
    }
    ec = pthread_mutexattr_destroy(&attr);
    if (ec)
    {
        message = "pthread_mutexattr_destroy";
        goto OnSystemError;
    }

    return;
OnSystemError:
    throw std::system_error{ec, std::system_category(), message};
}

section::~section() noexcept
{
    auto* osx = for_osx(this);
    auto* mtx = std::addressof(osx->mtx);

    if (auto ec = pthread_mutex_destroy(mtx))
        std::fputs(std::system_error{ec, std::system_category(),
                                     "pthread_mutex_destroy"}
                       .what(),
                   stderr);
}

bool section::try_lock() noexcept
{
    auto* osx = for_osx(this);
    auto* mtx = std::addressof(osx->mtx);

    // EBUSY  // possible error
    // EINVAL
    return pthread_mutex_trylock(mtx) == 0;
}

// - Note
//
//  There was an issue with `pthread_mutex_`
//  it returned EINVAL for lock operation
//  replacing it the rwlock
//
void section::lock() noexcept(false)
{
    auto* osx = for_osx(this);
    auto* mtx = std::addressof(osx->mtx);

    if (auto ec = pthread_mutex_lock(mtx))
        // EINVAL ?
        throw std::system_error{ec, std::system_category(),
                                "pthread_mutex_lock"};
}

void section::unlock() noexcept(false)
{
    auto* osx = for_osx(this);
    auto* mtx = std::addressof(osx->mtx);

    if (auto ec = pthread_mutex_unlock(mtx))
        throw std::system_error{ec, std::system_category(),
                                "pthread_mutex_unlock"};
}
