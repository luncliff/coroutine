//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <cstdio>
#include <ctime>
#include <system_error>

#include <concurrency_helper.h>

using namespace std;
using namespace std::chrono;

void setup_mutex_attr(pthread_mutexattr_t& attr) noexcept(false) {
    if (auto ec = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK))
        throw system_error{ec, system_category(), "pthread_mutexattr_settype"};

// see pthread.h in /ndk/sysroot/usr/include
// general POSIX or NDK high version
#if !defined(__ANDROID_API__) ||                                               \
    (defined(__ANDROID_API__) && __ANDROID_API__ >= 28)

    if (auto ec = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE))
        throw system_error{ec, system_category(),
                           "pthread_mutexattr_setprotocol"};
#endif
}

latch::latch(uint32_t count) noexcept(false) : ref{count}, cv{}, mtx{} {
    // increase count on ctor phase

    // instread of std::mutex, use customized attributes
    pthread_mutexattr_t attr{};
    if (auto ec = pthread_mutexattr_init(&attr))
        throw system_error{ec, system_category(), "pthread_mutexattr_init"};

    setup_mutex_attr(attr);

    if (auto ec = pthread_mutex_init(addressof(this->mtx), &attr))
        throw system_error{ec, system_category(), "pthread_mutex_init"};

    if (auto ec = pthread_mutexattr_destroy(&attr))
        throw system_error{ec, system_category(), "pthread_mutexattr_destroy"};

    if (auto ec = pthread_cond_init(addressof(this->cv), nullptr))
        throw system_error{ec, system_category(), "pthread_cond_init"};
}

latch::~latch() noexcept {
    try {
        if (auto ec = pthread_cond_destroy(addressof(this->cv)))
            throw system_error{ec, system_category(), "pthread_cond_destroy"};

        if (auto ec = pthread_mutex_destroy(addressof(this->mtx)))
            throw system_error{ec, system_category(), "pthread_mutex_destroy"};
    } catch (const system_error& ex) {
        fputs(ex.what(), stderr);
    }
}
void latch::count_down_and_wait() noexcept(false) {
    this->count_down();
    this->wait();
}
void latch::count_down(uint32_t n) noexcept(false) {
    // underflow
    if (n > this->ref.load(memory_order_acquire))
        throw underflow_error{"latch::count_down"};

    this->ref.fetch_sub(n, memory_order_release);
    if (this->ref.load(memory_order_acquire) != 0)
        return;
    // notify error
    if (auto ec = pthread_cond_signal(&this->cv))
        throw system_error{ec, system_category(), "pthread_cond_signal"};
}
bool latch::is_ready() const noexcept {
    return this->ref.load(memory_order_acquire) == 0;
}
int latch::timed_wait(nanoseconds timeout) noexcept {
    // `pthread_cond_timedwait` uses absolute time(== time point).
    // so we have to calculate the timepoint first
    auto end_time = 0ns;
    auto until = timespec{};
    const auto invoke_time = system_clock::now().time_since_epoch();
    int reason = 0;

    end_time = invoke_time + timeout;
    until.tv_sec = duration_cast<seconds>(end_time).count();
    until.tv_nsec = duration_cast<nanoseconds>(end_time).count() % 1'000'000;

CheckCurrentCount:
    if (this->is_ready())
        return 0;

    // This check is for possible spurious wakeup.
    auto current_time = system_clock::now().time_since_epoch();
    if (end_time < current_time)
        // reached timeout. return error
        return reason;
    {
        if (auto ec = pthread_mutex_lock(addressof(this->mtx)))
            return ec;
        reason = pthread_cond_timedwait(addressof(this->cv),
                                        addressof(this->mtx), addressof(until));
        if (auto ec = pthread_mutex_unlock(addressof(this->mtx)))
            return ec;
    }
    if (reason == ETIMEDOUT) // this might be a spurious wakeup
        goto CheckCurrentCount;

    // reason containes error code at this moment
    return reason;
}
void latch::wait() noexcept(false) {
    auto timeout = 1536ms;
    while (true) {
        // since latch doesn't provide interface for waiting with timeout,
        // this code will wait more if it reached timeout
        auto ec = this->timed_wait(timeout);
        if (ec == 0)
            return;
        if (ec != ETIMEDOUT)
            throw system_error{static_cast<int>(ec), system_category()};

        // ... update timeout if it need to be changed ...
    }
}
