//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"
#include <iostream>

using namespace coro;

struct run_on_pthread final {
    pthread_t* const id;
    const pthread_attr_t* const attr;

  public:
    explicit run_on_pthread(pthread_t& _tid, const pthread_attr_t* _attr)
        : id{addressof(_tid)}, attr{_attr} {};

    static auto resume_by_pthread(void* ptr) noexcept -> void* {
        coroutine_handle<void>::from_address(ptr).resume();
        return nullptr;
    }

  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(coroutine_handle<void> rh) const noexcept(false) {
        if (auto ec = pthread_create(id, attr, //
                                     resume_by_pthread, rh.address()))
            throw system_error{ec, system_category(), "pthread_create"};
    }
    void await_resume() const noexcept {
        assert(*id == pthread_self());
    }
};

auto sleep_in_background(pthread_t& tid, //
                         const pthread_attr_t* attr) noexcept(false)
    -> no_return {
    // this line is foreground (spawner) thread
    const auto before = pthread_self();

    co_await run_on_pthread(tid, attr);

    // after co_await, `pthread_self` will return background thread's id
    if (before == pthread_self())
        throw logic_error{
            "pthread_self in background returned foreground thread's id"};
}

auto spwan_and_join_pthread() {
    pthread_t tid{};
    try {
        sleep_in_background(tid, nullptr);

        if (auto ec = pthread_join(tid, nullptr))
            throw system_error{ec, system_category(), "pthread_join"};

    } catch (const exception& ex) {
        FAIL_WITH_MESSAGE(string{ex.what()});
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return spwan_and_join_pthread();
}
#endif
