﻿// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

using namespace std;

namespace concrt {

section::section() noexcept(false) : rwlock{} {
    if (auto ec = pthread_rwlock_init(&rwlock, nullptr))
        throw system_error{ec, system_category(), "pthread_rwlock_init"};
}

section::~section() noexcept {
    try {
        if (auto ec = pthread_rwlock_destroy(&rwlock))
            throw system_error{ec, system_category(), "pthread_rwlock_init"};
    } catch (const system_error& e) {
        ::perror(e.what());
    } catch (...) {
        ::perror("Unknown exception in section dtor");
    }
}

bool section::try_lock() noexcept {
    // EBUSY  // possible error
    // EINVAL
    // EDEADLK
    auto ec = pthread_rwlock_trywrlock(&rwlock);
    return ec == 0;
}

// - Note
//
//  There was an issue with `pthread_mutex_`
//  it returned EINVAL for lock operation
//  replacing it the rwlock
//
void section::lock() noexcept(false) {
    if (auto ec = pthread_rwlock_wrlock(&rwlock))
        // EINVAL ?
        throw system_error{ec, system_category(), "pthread_rwlock_wrlock"};
}

void section::unlock() noexcept(false) {
    if (auto ec = pthread_rwlock_unlock(&rwlock))
        throw system_error{ec, system_category(), "pthread_rwlock_unlock"};
}
} // namespace concrt

// check the compiler supports coroutine instructions
#if defined(__clang__)
// in-use functions
static_assert(__has_builtin(__builtin_coro_done));
static_assert(__has_builtin(__builtin_coro_resume));
static_assert(__has_builtin(__builtin_coro_destroy));
// known functions
static_assert(__has_builtin(__builtin_coro_promise));
static_assert(__has_builtin(__builtin_coro_size));
static_assert(__has_builtin(__builtin_coro_frame));
static_assert(__has_builtin(__builtin_coro_free));
static_assert(__has_builtin(__builtin_coro_id));
static_assert(__has_builtin(__builtin_coro_begin));
static_assert(__has_builtin(__builtin_coro_end));
static_assert(__has_builtin(__builtin_coro_suspend));
static_assert(__has_builtin(__builtin_coro_param));
#endif