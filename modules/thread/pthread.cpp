//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/thread.h>

namespace coro {

void* coroutine_pthread_resume(void* ptr) noexcept(false) {
    // assume we will receive an address of the coroutine frame
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done() == false)
        task.resume();

    return task.address(); // coroutine_handle<void>::address();
}

void pthread_spawner_t::resume_on_pthread(coroutine_handle<void> rh) //
    noexcept(false) {
    if (auto ec = pthread_create(this->tid, this->attr,
                                 coroutine_pthread_resume, rh.address()))
        throw system_error{ec, system_category(), "pthread_create"};
}

pthread_joiner_t::pthread_joiner_t(promise_type* p) noexcept(false)
    : pthread_knower_t{p} {
    if (p == nullptr)
        throw invalid_argument{"nullptr for promise_type*"};
}

void pthread_joiner_t::try_join() noexcept(false) {
    pthread_t tid = *this;
    if (tid == pthread_t{}) // spawned no threads. nothing to do
        return;

    void* ptr{};
    // we must acquire `tid` before the destruction
    if (auto ec = pthread_join(tid, &ptr)) {
        throw system_error{ec, system_category(), "pthread_join"};
    }
    if (auto frame = coroutine_handle<void>::from_address(ptr)) {
        frame.destroy();
    }
}

pthread_detacher_t::pthread_detacher_t(promise_type* p) noexcept(false)
    : pthread_knower_t{p} {
    if (p == nullptr)
        throw invalid_argument{"nullptr for promise_type*"};
}

void pthread_detacher_t::try_detach() noexcept(false) {
    pthread_t tid = *this;
    if (tid == pthread_t{}) // spawned no threads. nothing to do
        return;

    if (auto ec = pthread_detach(tid)) {
        throw system_error{ec, system_category(), "pthread_join"};
    }
}

} // namespace coro
