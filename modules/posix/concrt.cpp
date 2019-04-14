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

namespace concrt
{
struct latch_posix final
{
    atomic<uint32_t> count{};
    pthread_cond_t cv{};
    pthread_mutex_t mtx{};
};

auto for_posix(latch* wg) noexcept -> latch_posix*
{
    static_assert(sizeof(latch_posix) <= sizeof(latch));
    return reinterpret_cast<latch_posix*>(wg);
}
auto for_posix(const latch* wg) noexcept -> const latch_posix*
{
    return for_posix(const_cast<latch*>(wg));
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
    this->count_down();
    this->wait();
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

    // notify error
    if (auto ec = pthread_cond_signal(&wg->cv))
        throw system_error{ec, system_category(), "pthread_cond_signal"};
}

bool latch::is_ready() const noexcept
{
    const auto wg = for_posix(this);
    return wg->count.load(memory_order_acquire) == 0;
}

std::errc timed_wait(latch_posix& wg, microseconds timeout) noexcept
{
    // check count fast
    if (wg.count.load(memory_order_relaxed) == 0)
        return std::errc{};

    // `pthread_cond_timedwait` uses absolute time(== time point).
    // so we have to calculate the timepoint first
    auto end_time = 0us;
    auto until = timespec{};
    const auto invoke_time = system_clock::now().time_since_epoch();

    end_time = invoke_time + timeout;
    until.tv_sec = duration_cast<seconds>(end_time).count();
    until.tv_nsec = duration_cast<nanoseconds>(end_time).count() % 1'000'000;

CheckCurrentCount:
    if (wg.count.load(memory_order_acquire) == 0)
        return std::errc{};

    // This check is for possible spurious wakeup.
    auto current_time = system_clock::now().time_since_epoch();
    if (end_time < current_time)
        // reached timeout. return error
        return std::errc{ETIMEDOUT};

    int reason = 0;
    {
        if (auto ec = pthread_mutex_lock(addressof(wg.mtx)))
            return std::errc{ec};
        reason = pthread_cond_timedwait(addressof(wg.cv), addressof(wg.mtx),
                                        addressof(until));
        if (auto ec = pthread_mutex_unlock(addressof(wg.mtx)))
            return std::errc{ec};
    }
    if (reason == ETIMEDOUT) // this might be a spurious wakeup
        goto CheckCurrentCount;

    // reason containes error code at this moment
    return std::errc{reason};
}

void latch::wait() noexcept(false)
{
    auto wg = for_posix(this);
    auto timeout = 100ms;

    while (true)
    {
        // since latch doesn't provide interface for waiting with timeout,
        // this code will wait more if it reached timeout
        auto ec = timed_wait(*wg, timeout);
        if (ec == std::errc{})
            return;
        if (ec != errc::timed_out)
            throw system_error{static_cast<int>(ec), system_category()};

        // ... update timeout if it need to be changed ...
    }
}

// barrier::barrier(uint32_t num_threads) noexcept(false) : storage{}
// {
//     throw runtime_error{"not implemented"};
// }
// barrier::~barrier() noexcept
// {
// }
// void barrier::arrive_and_wait() noexcept
// {
//     throw runtime_error{"not implemented"};
// }

} // namespace concrt
