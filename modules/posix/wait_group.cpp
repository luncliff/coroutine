// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <ctime>
#include <system_error>

#include <pthread.h>

using namespace std;
using namespace std::chrono;

struct wait_group_posix final
{
    atomic<uint32_t> count{};
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};
};

auto for_posix(wait_group* wg) noexcept
{
    static_assert(sizeof(wait_group_posix) <= sizeof(wait_group));
    return reinterpret_cast<wait_group_posix*>(wg);
}

wait_group::wait_group() noexcept(false) : storage{}
{
    auto* wg = for_posix(this);

    new (wg) wait_group_posix{};

    // init attr?
    if (auto ec = pthread_mutex_init(addressof(wg->mtx), nullptr))
        throw system_error{ec, system_category(), "pthread_mutex_init"};

    if (auto ec = pthread_cond_init(addressof(wg->cv), nullptr))
        throw system_error{ec, system_category(), "pthread_cond_init"};
}

wait_group::~wait_group() noexcept
{
    auto* wg = for_posix(this);
    try
    {
        if (auto ec = pthread_cond_destroy(addressof(wg->cv)))
            throw system_error{ec, system_category(), "pthread_cond_destroy"};

        if (auto ec = pthread_mutex_destroy(addressof(wg->mtx)))
            throw system_error{ec, system_category(), "pthread_mutex_destroy"};
    }
    catch (const system_error& ex)
    {
        fputs(ex.what(), stderr);
    }
}

void wait_group::add(uint16_t delta) noexcept
{
    auto* wg = for_posix(this);

    // just increase count.
    // notice that delta is always positive
    wg->count.fetch_add(delta);
}

void wait_group::done() noexcept
{
    auto* wg = for_posix(this);

    if (wg->count > 0)
        wg->count.fetch_sub(1);

    if (wg->count.load() != 0)
        return;

    // notify
    pthread_cond_signal(&wg->cv);
}

bool wait_group::wait(duration timeout) noexcept(false)
{
    auto wg = for_posix(this);
    auto abs_time = microseconds{};
    auto until = timespec{};
    auto reason = 0;

    // check count first
    if (wg->count.load(memory_order_relaxed) == 0)
        return true;

    // timedwait uses absolute time.
    // so we have to calculate the timepoin first
    abs_time = (system_clock::now() + timeout).time_since_epoch();
    until.tv_sec = duration_cast<seconds>(abs_time).count();
    until.tv_nsec = duration_cast<nanoseconds>(abs_time).count() % 1'000'000;

CheckCurrentTime:
    // check count again
    if (wg->count.load(memory_order_acquire) == 0)
        return true;

    // This loop is for possible spurious wakeup.
    // Wait if there is more time
    if (abs_time < system_clock::now().time_since_epoch())
        return false;

    if (auto ec = pthread_mutex_lock(addressof(wg->mtx)))
        throw system_error{ec, system_category(), "pthread_mutex_lock"};

    reason = pthread_cond_timedwait(addressof(wg->cv), addressof(wg->mtx),
                                    addressof(until));

    if (auto ec = pthread_mutex_unlock(addressof(wg->mtx)))
        throw system_error{ec, system_category(), "pthread_mutex_unlock"};

    if (reason == ETIMEDOUT)
        // check the time again. is it spurious wakeup?
        goto CheckCurrentTime;

    // reason containes error code at this moment
    return reason == 0;
}
