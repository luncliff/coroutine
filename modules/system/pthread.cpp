/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/pthread.h>

namespace coro {

void* pthread_spawner_t::pthread_resume(void* ptr) noexcept(false) {
    // assume we will receive an address of the coroutine frame
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done() == false)
        task.resume();

    return task.address(); // coroutine_handle<void>::address();
}

void pthread_spawner_t::resume_on_pthread(coroutine_handle<void> rh) //
    noexcept(false) {
    if (auto ec = pthread_create(this->tid, this->attr, //
                                 pthread_resume, rh.address()))
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

class section final {
  private:
    pthread_rwlock_t rwlock;

  public:
    section() noexcept(false);
    ~section() noexcept;
    section(const section&) = delete;
    section(section&&) = delete;
    section& operator=(const section&) = delete;
    section& operator=(section&&) = delete;

    bool try_lock() noexcept;
    void lock() noexcept(false);
    void unlock() noexcept(false);
};

/**
 * @see pthread_rwlock_init
 */
section::section() noexcept(false) : rwlock{} {
    if (auto ec = pthread_rwlock_init(&rwlock, nullptr))
        throw system_error{ec, system_category(), "pthread_rwlock_init"};
}

/**
 * @see pthread_rwlock_destroy
 */
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
/**
 * possible errors are ...
 * * EBUSY
 * * EINVAL
 * * EDEADLK
 * 
 * @see pthread_rwlock_trywrlock
 * @return true 
 * @return false failed. check `errno`
 */
bool section::try_lock() noexcept {
    auto ec = pthread_rwlock_trywrlock(&rwlock);
    return ec == 0;
}

/**
 * @note `pthread_mutex_` returned EINVAL for lock operation. replaced to rwlock
 * @see pthread_rwlock_wrlock
 */
void section::lock() noexcept(false) {
    if (auto ec = pthread_rwlock_wrlock(&rwlock))
        // EINVAL ?
        throw system_error{ec, system_category(), "pthread_rwlock_wrlock"};
}

/**
 * @see pthread_rwlock_unlock
 */
void section::unlock() noexcept(false) {
    if (auto ec = pthread_rwlock_unlock(&rwlock))
        throw system_error{ec, system_category(), "pthread_rwlock_unlock"};
}

} // namespace coro
