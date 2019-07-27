//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

namespace coro {

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

} // namespace coro
