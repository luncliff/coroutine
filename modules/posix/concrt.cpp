// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <system_error>

#include <pthread.h>

using namespace std;
using namespace std::chrono;

struct latch_posix final
{
    atomic<uint32_t> count{};
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};
};

namespace concrt
{

auto for_posix(latch* wg) noexcept -> latch_posix*
{
    static_assert(sizeof(latch_posix) <= sizeof(latch));
    return reinterpret_cast<latch_posix*>(wg);
}

latch::latch(uint32_t count) noexcept(false) : storage{}
{
    auto wg = new (for_posix(this)) latch_posix{};

    // init attr?
    if (auto ec = pthread_mutex_init(addressof(wg->mtx), nullptr))
        throw system_error{ec, system_category(), "pthread_mutex_init"};

    if (auto ec = pthread_cond_init(addressof(wg->cv), nullptr))
        throw system_error{ec, system_category(), "pthread_cond_init"};

    // just increase count.
    // notice that delta is always positive
    wg->count.fetch_add(count);
}

latch::~latch() noexcept
{
    auto wg = for_posix(this);
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
void latch::count_down_and_wait() noexcept(false)
{
    return this->count_down(), this->wait();
}

void latch::count_down(uint32_t n) noexcept(false)
{
    auto wg = for_posix(this);

    // underflow
    if (n > wg->count.load(memory_order_acquire))
        throw underflow_error{"latch::count_down"};

    wg->count.fetch_sub(n, memory_order_release);
    if (wg->count.load(memory_order_acquire) != 0)
        return;

    // notify
    pthread_cond_signal(&wg->cv);
}

void latch::wait() noexcept(false)
{
    auto wg = for_posix(this);

    // check count first
    if (wg->count.load(memory_order_relaxed) == 0)
        return;

    // since latch doesn't provide interface for waiting with timeout,
    // this code will wait more if it reached timeout
    auto timeout = 100ms;

    // `pthread_cond_timedwait` uses absolute time(time point).
    // so we have to calculate the timepoint first
    auto abs_time = 0us;
    auto until = timespec{};
CheckCurrentTime:
    auto until_timepoint = system_clock::now();
    // This check is for possible spurious wakeup.
    // Wait if there is more time
    if (abs_time < until_timepoint.time_since_epoch())
    {
        // istead of throw exception for spurious_wakeup,
        // add more timeout and try again.
        abs_time = (until_timepoint + timeout).time_since_epoch();
        until.tv_sec = duration_cast<seconds>(abs_time).count();
        until.tv_nsec
            = duration_cast<nanoseconds>(abs_time).count() % 1'000'000;
    }

    // CheckCurrentCount:
    if (wg->count.load(memory_order_acquire) == 0)
        return;

    if (auto ec = pthread_mutex_lock(addressof(wg->mtx)))
        throw system_error{ec, system_category(), "pthread_mutex_lock"};

    auto reason = pthread_cond_timedwait(addressof(wg->cv), addressof(wg->mtx),
                                         addressof(until));

    if (auto ec = pthread_mutex_unlock(addressof(wg->mtx)))
        throw system_error{ec, system_category(), "pthread_mutex_unlock"};

    if (reason == ETIMEDOUT)
        goto CheckCurrentTime; // this might be a spurious wakeup

    // reason containes error code at this moment
    if (auto ec = reason)
        throw system_error{ec, system_category(), "pthread_cond_timedwait"};
}

} // namespace concrt
