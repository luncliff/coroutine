//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/return.h>
#include <coroutine/thread.h>

#include <concurrency_helper.h>

#include "test.h"
using namespace std;
using namespace coro;

auto work_on_win32_thread_pool(atomic_uint32_t& counter, latch& wg)
    -> forget_frame {
    co_await ptp_work{};
    ++counter;
    wg.count_down();
}

auto win32_ptp_work_fork_join() {
    constexpr auto num_worker = 40u;

    latch wg{num_worker};
    atomic_uint32_t counter = 0;

    for (auto i = 0; i < num_worker; ++i) {
        work_on_win32_thread_pool(counter, wg);
    }

    try {
        wg.wait();
        _require_(counter == num_worker);

    } catch (const system_error& e) {
        _fail_now_(e.what(), __FILE__, __LINE__);
    }
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char*[]) {
    return win32_ptp_work_fork_join();
}

#elif __has_include(<CppUnitTest.h>)
#include <CppUnitTest.h>

template <typename T>
using TestClass = ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<T>;

class coro_win32_ptp_work_fork_join
    : public TestClass<coro_win32_ptp_work_fork_join> {
    TEST_METHOD(test_win32_ptp_work_fork_join) {
        win32_ptp_work_fork_join();
    }
};
#endif
