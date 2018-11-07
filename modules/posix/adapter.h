

#ifndef PTHREAD_ADAPTER_H
#define PTHREAD_ADAPTER_H
#include <pthread.h>

struct posix_mutex_t final
{
    pthread_mutex_t mtx{};

    posix_mutex_t() noexcept(false)
    {
        auto ec = pthread_mutex_init(&mtx, nullptr);
        assert(ec == 0);
    }
    ~posix_mutex_t() noexcept { pthread_mutex_destroy(&mtx); }

  public:
    void lock() noexcept
    {
        auto ec = pthread_mutex_lock(&mtx);
        assert(ec == 0);
    }
    void unlock() noexcept
    {
        auto ec = pthread_mutex_unlock(&mtx);
        assert(ec == 0);
    }
    bool try_lock() noexcept
    {
        auto ec = pthread_mutex_trylock(&mtx);
        return ec == 0;
    }
};
static_assert(sizeof(posix_mutex_t) == sizeof(pthread_mutex_t));

struct posix_cond_var final
{
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};
    pthread_condattr_t attr{};

  public:
    posix_cond_var() noexcept(false)
    {
        // init attr?
        pthread_mutex_init(&mtx, nullptr);
        pthread_cond_init(&cv, nullptr);
    }
    ~posix_cond_var() noexcept
    {
        // destry attr?
        pthread_mutex_destroy(&mtx);
        pthread_cond_destroy(&cv);
    }

  public:
    void wait(std::uint32_t ms) noexcept
    {
        timespec timeout{};
        timeout.tv_nsec = ms * 1'000'000; // ms -> nano

        pthread_mutex_lock(&mtx);
        pthread_cond_timedwait(&cv, &mtx, &timeout);
        pthread_mutex_unlock(&mtx);
    }
    void signal() noexcept
    {
        // expect some unknown error. ignore
        pthread_cond_signal(&cv);
    }
};

#endif // PTHREAD_ADAPTER_H