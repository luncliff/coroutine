//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>
#include <cstdio>

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
            throw system_error{ec, system_category(), "pthread_rwlock_destroy"};
    } catch (const system_error& e) {
        perror(e.what());
    } catch (...) {
        perror("Unknown exception in section dtor");
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

void* pthread_joiner_t::pthread_spawner_t::resume_on_pthread(
    void* ptr) noexcept(false) {

    // assume we will receive an address of the coroutine frame
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done() == false)
        task.resume();

    return task.address(); // coroutine_handle<void>::address();
}

void pthread_joiner_t::pthread_spawner_t::on_suspend(
    coroutine_handle<void> rh) noexcept(false) {
    if (auto ec = pthread_create(this->tid, this->attr, resume_on_pthread,
                                 rh.address()))
        throw system_error{ec, system_category(), "pthread_create"};
}

// we will receive the pointer from `get_return_object`
pthread_joiner_t::pthread_joiner_t(promise_type* p) noexcept(false)
    : promise{p} {
    if (p == nullptr)
        throw invalid_argument{"nullptr for promise_type*"};
}

pthread_joiner_t::~pthread_joiner_t() noexcept(false) {
    if (promise->tid == pthread_t{}) // spawned no threads. nothing to do
        return;

    void* ptr{};
    // we must acquire `tid` before the destruction
    if (auto ec = pthread_join(promise->tid, &ptr)) {
        throw system_error{ec, system_category(), "pthread_join"};
    }
    if (auto frame = coroutine_handle<void>::from_address(ptr)) {
        frame.destroy();
    }
}

} // namespace concrt
