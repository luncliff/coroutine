/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/pthread.h>

namespace coro {
using namespace std;

void* continue_on_pthread::on_pthread(void* ptr) noexcept(false) {
    // assume we will receive an address of the coroutine frame
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done() == false)
        task.resume();

    return task.address(); // coroutine_handle<void>::address();
}

uint32_t
continue_on_pthread::spawn(pthread_t& tid, const pthread_attr_t* attr,
                           coroutine_handle<void> coro) noexcept(false) {
    return ::pthread_create(&tid, attr, on_pthread, coro.address());
}

pthread_joiner::pthread_joiner(promise_type* p) noexcept(false) : promise{p} {
    if (p == nullptr)
        throw invalid_argument{"nullptr for promise_type*"};
}

pthread_joiner::~pthread_joiner() noexcept(false) {
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

pthread_detacher::pthread_detacher(promise_type* p) noexcept(false)
    : promise{p} {
    if (p == nullptr)
        throw invalid_argument{"nullptr for promise_type*"};
}

pthread_detacher::~pthread_detacher() noexcept(false) {
    pthread_t tid = *this;
    if (tid == pthread_t{}) // spawned no threads. nothing to do
        return;

    if (auto ec = pthread_detach(tid)) {
        throw system_error{ec, system_category(), "pthread_join"};
    }
}

} // namespace coro
