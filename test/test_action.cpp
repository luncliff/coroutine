/**
 * @author  github.com/luncliff (luncliff@gmail.com)
 * @see     https://gist.github.com/luncliff/1fedae034c001a460e9233ecf0afc25b
 */
#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#if __has_include(<dispatch/dispatch.h>)
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#endif
#if __has_include(<catch2/catch_all.hpp>)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include <spdlog/spdlog.h>

#include "action.hpp"

using namespace coro;
using namespace std::chrono_literals;
using namespace std::chrono;

struct fire_and_forget_test_case {
    /**
     * @details Compiler will warn if this function is `noexcept`.
     *  However, it will be handled by the return type. 
     *  The uncaught exception won't happen here.
     * 
     * @see fire_and_forget::unhandled_exception 
     */
    static fire_and_forget throw_in_coroutine() noexcept(false) {
        co_await suspend_never{};
        throw std::runtime_error{__func__};
    }
};

TEST_CASE_METHOD(fire_and_forget_test_case, "unhandled exception", "[exception]") {
    REQUIRE_NOTHROW(throw_in_coroutine());
}

#if __has_include(<dispatch/dispatch.h>)

struct paused_action_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  public:
    static paused_action_t spawn0() {
        co_await suspend_never{};
        spdlog::debug("{}", __func__);
        co_return;
    }
};

TEST_CASE_METHOD(paused_action_test_case, "synchronous dispatch", "[dispatch]") {
    paused_action_t action = spawn0();
    coroutine_handle<void> coro = action.handle();
    REQUIRE(coro);
    REQUIRE_FALSE(coro.done());
    // these are synchronous operations with a `dispatch_queue_t`
    SECTION("dispatch_async_and_wait") {
        dispatch_async_and_wait_f(queue, coro.address(), resume_once);
        spdlog::debug("dispatch_async_and_wait");
        REQUIRE(coro.done());
    }
    SECTION("dispatch_barrier_async_and_wait") {
        dispatch_barrier_async_and_wait_f(queue, coro.address(), resume_once);
        spdlog::debug("dispatch_barrier_async_and_wait");
        REQUIRE(coro.done());
    }
    SECTION("dispatch_sync") {
        dispatch_sync_f(queue, coro.address(), resume_once);
        spdlog::debug("dispatch_sync");
        REQUIRE(coro.done());
    }
}

struct queue_awaitable_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  public:
    /**
     * @details signal the given semaphore when the coroutine is finished
     */
    static fire_and_forget spawn1(dispatch_queue_t queue, dispatch_semaphore_t semaphore) {
        co_await queue_awaitable_t{queue};
        spdlog::trace("{}", __func__);
        dispatch_semaphore_signal(semaphore);
        co_return;
    }

    /**
     * @brief With `queue_awaitable_t`, this routine will be resumed by a worker of `dispatch_queue_t`
     */
    static paused_action_t spawn2(dispatch_queue_t queue) {
        co_await queue_awaitable_t{queue};
        spdlog::trace("{}", __func__);
    }
    /**
     * @details spawn3 will await spawn2. By use of paused_action_t,
     *          `spawn3`'s handle becomes the 'next' of `spawn2`.
     *          When `spawn2` returns, `spawn3` will be resumed by `end_awaitable_t`.
     */
    static paused_action_t spawn3(dispatch_queue_t queue) {
        spdlog::trace("{} start", __func__);
        co_await spawn2(queue);
        spdlog::trace("{} end", __func__);
    }
};

TEST_CASE_METHOD(queue_awaitable_test_case, "await and semaphore wait", "[dispatch][semaphore]") {
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    spawn1(queue, semaphore);
    // wait for the semaphore signal for 10 ms
    const auto d = duration_cast<nanoseconds>(10ms);
    const dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, d.count());
    REQUIRE(dispatch_semaphore_wait(semaphore, timeout) == 0);
}

TEST_CASE_METHOD(queue_awaitable_test_case, "await and busy wait", "[dispatch]") {
    paused_action_t action = spawn3(queue);
    coroutine_handle<void> task = action.handle();
    REQUIRE_NOTHROW(task.resume());
    // spawn2 will be resumed by GCD
    while (task.done() == false)
        std::this_thread::sleep_for(1ms);
}

TEST_CASE("semaphore lifecycle", "[dispatch][semaphore]") {
    SECTION("create, retain, release, release") {
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        dispatch_retain(sem);
        dispatch_release(sem);
        dispatch_release(sem); // sem is dead.
        // dispatch_release(sem); // BAD instruction
    }
    SECTION("create, release") {
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        dispatch_release(sem); // sem is deamd
        // dispatch_release(sem); // BAD instruction
    }
}

/// @see https://developer.apple.com/documentation/dispatch/1452955-dispatch_semaphore_create
TEST_CASE("semaphore signal/wait", "[dispatch][semaphore]") {
    SECTION("value 0") {
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        dispatch_semaphore_signal(sem);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW) != 0); // timeout
        dispatch_release(sem);
    }
    SECTION("value 1") {
        dispatch_semaphore_t sem = dispatch_semaphore_create(1);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW) != 0);
        // At this point, the semaphore's value is 0.
        // `dispatch_release` in this state will lead to EXC_BAD_INSTRUCTION.
        // Increase the `value` to that of `dispatch_semaphore_create`
        dispatch_semaphore_signal(sem);
        dispatch_release(sem);
    }
    SECTION("value 3") {
        dispatch_semaphore_t sem = dispatch_semaphore_create(3);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW) != 0);

        // Same as above. But this time the value is 3. We have to signal again and again
        dispatch_semaphore_signal(sem);
        dispatch_semaphore_signal(sem);
        dispatch_semaphore_signal(sem); // back to 3

        // if `dispatch_semaphore_signal` is not enough, BAD instruction.
        dispatch_release(sem);
    }
}

TEST_CASE("semaphore_owner_t", "[dispatch][semaphore][lifecycle]") {
    semaphore_owner_t sem{};
    WHEN("untouched") {
        // do nothing with the instance
    }
    WHEN("copy construction") {
        semaphore_owner_t sem2{sem};
    }
    GIVEN("signaled") {
        REQUIRE_NOTHROW(sem.signal());
        WHEN("wait/wait_for") {
            REQUIRE(sem.wait());
            REQUIRE_FALSE(sem.wait_for(1s)); // already consumed by previous wait
        }
        WHEN("wait_for/wait_for") {
            REQUIRE(sem.wait_for(1s));
            REQUIRE_FALSE(sem.wait_for(0s)); // already consumed by previous wait
        }
    }
    GIVEN("not-signaled")
    WHEN("wait_for") {
        REQUIRE_FALSE(sem.wait_for(0s));
    }
}

struct semaphore_action_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    static fire_and_forget sleep_and_signal(dispatch_queue_t queue, //
                                            dispatch_semaphore_t handle, std::chrono::seconds duration) {
        semaphore_owner_t sem{handle}; // guarantee life of the semaphore with retain/release
        co_await queue_awaitable_t{queue};
        std::this_thread::sleep_for(duration);
        spdlog::debug("signal: {}", (void*)sem.handle());
        sem.signal(); // == dispatch_semaphore_signal
    };

    /// @note the order of the arguments
    static semaphore_action_t sleep_and_return(dispatch_semaphore_t sem, dispatch_queue_t queue,
                                               std::chrono::seconds duration) {
        co_await queue_awaitable_t{queue};
        std::this_thread::sleep_for(duration);
        spdlog::debug("signal: {}", (void*)sem);
        co_return; // `promise_type::return_void` will signal the semaphore
    };
};

TEST_CASE_METHOD(semaphore_action_test_case, "launch and wait", "[dispatch][semaphore]") {
    semaphore_action_t action = sleep_and_return(dispatch_semaphore_create(0), queue, 1s);
    spdlog::debug("wait: {}", "begin");
    REQUIRE_FALSE(action.wait_for(0s)); // there is a sleep, so this must fail
    REQUIRE(action.wait());
    spdlog::debug("wait: {}", "end");
}

TEST_CASE_METHOD(semaphore_action_test_case, "wait first action", "[dispatch][semaphore]") {
    auto cleanup = [](dispatch_semaphore_t handle) {
        spdlog::debug("wait: {}", (void*)handle);
        // ensure all coroutines are finished in this section
        std::this_thread::sleep_for(3s);
    };
    semaphore_owner_t sem{};
    SECTION("manual") {
        sleep_and_signal(queue, sem.handle(), 1s);
        sleep_and_signal(queue, sem.handle(), 2s);
        sleep_and_signal(queue, sem.handle(), 3s);
        REQUIRE(sem.wait());
        cleanup(sem.handle());
    }
    SECTION("signal with co_return") {
        sleep_and_return(sem.handle(), queue, 1s);
        sleep_and_return(sem.handle(), queue, 2s);
        sleep_and_return(sem.handle(), queue, 3s);
        REQUIRE(sem.wait());
        cleanup(sem.handle());
    }
}

struct group_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_group_t group = dispatch_group_create();

  public:
    group_test_case() {
        REQUIRE(group); // dispatch_group_create may return NULL
    }
    ~group_test_case() {
        dispatch_release(group);
    }

    static paused_action_t leave_when_return(dispatch_group_t group) {
        co_await suspend_never{};
        spdlog::debug("leave_when_return");
        co_return dispatch_group_leave(group);
    }

    static paused_action_t wait_and_signal(dispatch_group_t group, dispatch_semaphore_t sem) {
        co_await suspend_never{};
        spdlog::debug("wait_and_signal");
        auto ec = dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
        spdlog::debug("wait_and_signal: {}", ec);
        dispatch_semaphore_signal(sem);
        co_return;
    }
};

TEST_CASE_METHOD(group_test_case, "dispatch_group lifecycle", "[dispatch][group]") {
    SECTION("release without retain") {
        std::this_thread::sleep_for(10ms);
    }
    SECTION("additional retain, release") {
        dispatch_retain(group);
        std::this_thread::sleep_for(10ms);
        dispatch_release(group);
    }
}

TEST_CASE_METHOD(group_test_case, "dispatch_async_f + dispatch_group_async_f", "[dispatch][group]") {
    semaphore_owner_t sem{};
    paused_action_t t1 = leave_when_return(group);
    dispatch_group_enter(group); // group reference count == 1
    REQUIRE_FALSE(t1.handle().done());
    paused_action_t t2 = leave_when_return(group);
    dispatch_group_enter(group); // group reference count == 2
    REQUIRE_FALSE(t2.handle().done());

    paused_action_t t3 = wait_and_signal(group, sem.handle());
    dispatch_async_f(queue, t3.handle().address(), resume_once); // t3 will wait until the group is done

    dispatch_group_async_f(group, queue, t1.handle().address(), resume_once); // --> dispatch_group_wait
    dispatch_group_async_f(group, queue, t2.handle().address(), resume_once); // --> dispatch_group_wait
    REQUIRE(sem.wait_for(5s));
    REQUIRE(t1.handle().done());
    REQUIRE(t2.handle().done());
    REQUIRE(t3.handle().done());
}

struct group_action_test_case : public group_test_case {
  public:
    static group_action_t print_and_return([[maybe_unused]] dispatch_group_t, uint32_t line) {
        co_await suspend_never{};
        co_return spdlog::debug("print_and_return: {}", line);
    }
    static fire_and_forget wait_and_signal2(dispatch_group_t group, dispatch_queue_t queue, dispatch_semaphore_t sem) {
        spdlog::debug("wait_and_signal2");
        co_await group_awaitable_t{group, queue};
        spdlog::debug("wait_and_signal2: awake");
        dispatch_semaphore_signal(sem);
    }

    static group_action_t subtask1(dispatch_group_t, dispatch_queue_t queue) {
        co_await queue_awaitable_t{queue};
        spdlog::debug("subtask");
    }

    static semaphore_action_t wait_and_signal3(dispatch_semaphore_t, dispatch_group_t group, dispatch_queue_t queue) {
        uint32_t count = 10;
        while (count--) {
            group_action_t action = subtask1(group, queue);
            action.run_on(queue);
        }
        co_await group_awaitable_t{group, queue};
        co_return spdlog::debug("wait_and_signal3");
    }
};

TEST_CASE_METHOD(group_action_test_case, "wait group with dispatch_async_f", "[dispatch][group]") {
    semaphore_owner_t sem{};
    group_action_t action1 = print_and_return(group, __LINE__); // reference count == 1
    group_action_t action2 = print_and_return(group, __LINE__); // reference count == 2
    group_action_t action3 = print_and_return(group, __LINE__); // reference count == 3

    // it will wait (synchronously) until the group is done
    paused_action_t waiter = wait_and_signal(group, sem.handle());
    dispatch_async_f(queue, waiter.handle().address(), resume_once);

    action1.run_on(queue); // --> dispatch_group_wait
    action2.run_on(queue); // --> dispatch_group_wait
    action3.run_on(queue); // --> dispatch_group_wait

    REQUIRE(sem.wait_for(5s));
    REQUIRE(waiter.handle().done());
}

TEST_CASE_METHOD(group_action_test_case, "wait group with dispatch_group_notify_f", "[dispatch][group]") {
    semaphore_owner_t sem{};
    group_action_t action1 = print_and_return(group, __LINE__); // reference count == 1
    group_action_t action2 = print_and_return(group, __LINE__); // reference count == 2
    group_action_t action3 = print_and_return(group, __LINE__); // reference count == 3
    WHEN("await before schedules") {
        // wait until the group is completed
        wait_and_signal2(group, queue, sem.handle());
        action1.run_on(queue); // --> dispatch_group_wait
        action2.run_on(queue); // --> dispatch_group_wait
        action3.run_on(queue); // --> dispatch_group_wait
        REQUIRE(sem.wait());
    }
    WHEN("schedules before await") {
        action1.run_on(queue); // --> dispatch_group_wait
        action2.run_on(queue); // --> dispatch_group_wait
        action3.run_on(queue); // --> dispatch_group_wait
        std::this_thread::sleep_for(1s);
        // the group is already completed
        wait_and_signal2(group, queue, sem.handle());
        REQUIRE(sem.wait());
    }
}

TEST_CASE_METHOD(group_action_test_case, "wait group witt group_awaitable_t", "[dispatch][group]") {
    semaphore_action_t action = wait_and_signal3(dispatch_semaphore_create(0), group, queue);
    REQUIRE(action.wait());
}

struct timer_source_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_source_t timer = nullptr;
    std::atomic_uint32_t num_update = 0;
    std::atomic_uint32_t num_cancel = 0;

  public:
    timer_source_test_case() : timer{dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue)} {
        REQUIRE(timer);
        dispatch_retain(timer);
    }
    ~timer_source_test_case() {
        if (timer == nullptr)
            return;
        dispatch_release(timer);
    }

  public:
    static void on_timed_update(timer_source_test_case& t) noexcept {
        ++t.num_update;
        spdlog::debug("timer callback: {} {}", (void*)t.timer, t.num_update);
    }

    static void on_timed_canceled(timer_source_test_case& t) noexcept {
        ++t.num_cancel;
        spdlog::debug("timer canceled: {} {}", (void*)t.timer, t.num_cancel);
        dispatch_suspend(t.timer);
    }

    void reset_handlers() noexcept {
        dispatch_set_context(timer, this);
        dispatch_source_set_event_handler_f(timer, reinterpret_cast<dispatch_function_t>(on_timed_update));
        dispatch_source_set_cancel_handler_f(timer, reinterpret_cast<dispatch_function_t>(on_timed_canceled));
    }
};

SCENARIO_METHOD(timer_source_test_case, "dispatch timer", "[dispatch][timer]") {
    dispatch_set_context(timer, this);
    GIVEN("cancel handler") {
        dispatch_source_set_cancel_handler_f(timer, reinterpret_cast<dispatch_function_t>(on_timed_canceled));
        WHEN("cancel without resume") {
            dispatch_source_cancel(timer);
            // it's canced in suspended state
            // dispatch_sync_f(queue, noop_coroutine().address(), resume_once);
            std::this_thread::sleep_for(10ms);
            REQUIRE(num_cancel == 0);
        }
        WHEN("cancel with resume") {
            dispatch_resume(timer);
            dispatch_source_cancel(timer);
            std::this_thread::sleep_for(10ms);
            REQUIRE(num_cancel == 1);
        }
    }
    GIVEN("event handler") {
        const auto interval = duration_cast<nanoseconds>(20ms);
        dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, interval.count() / 2), interval.count(),
                                  (500us).count());
        dispatch_source_set_event_handler_f(timer, reinterpret_cast<dispatch_function_t>(on_timed_update));
        dispatch_resume(timer);
        std::this_thread::sleep_for(interval);
        REQUIRE(num_update.load() == 1);
        WHEN("release") {
            auto source = timer;
            dispatch_release(timer);
            timer = nullptr;
            std::this_thread::sleep_for(interval);
            REQUIRE(num_update.load() == 2); // timer still works
            dispatch_source_cancel(source);  // make sure no more invoke
        }
        WHEN("cancel") {
            dispatch_source_cancel(timer);
            std::this_thread::sleep_for(interval);
            REQUIRE(num_update.load() == 1); // no increase
        }
        WHEN("suspend without cancel") {
            dispatch_suspend(timer);
            std::this_thread::sleep_for(interval);
            REQUIRE(num_update.load() == 1); // no increase
        }
    }
}

struct timer_owner_test_case {
    std::atomic<uint32_t> num_update = 0;
    std::atomic<uint32_t> num_cancel = 0;

  public:
    static void on_timer_cancel(timer_owner_test_case& info) noexcept {
        ++info.num_cancel;
    }
    static void on_timer_event(timer_owner_test_case& info) noexcept {
        ++info.num_update;
    }
};

TEST_CASE_METHOD(timer_owner_test_case, "timer_owner_t", "[dispatch][timer]") {
    timer_owner_t timer{dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)};
    timer.set(this,                                                  //
              reinterpret_cast<dispatch_function_t>(on_timer_event), // num_update+1
              reinterpret_cast<dispatch_function_t>(on_timer_cancel) // num_cancel+1 and suspend
    );
    GIVEN("untouched") {
        WHEN("suspend once") {
            timer.suspend();
            REQUIRE(num_update == 0);
        }
        WHEN("cancel") {
            // not resumed. so we no cancel effect
            timer.cancel();
            std::this_thread::sleep_for(30ms);
            REQUIRE(num_cancel == 0);
        }
    }
    GIVEN("started") {
        timer.start(20ms, DISPATCH_TIME_NOW);
        std::this_thread::sleep_for(15ms);
        WHEN("suspend once") {
            timer.suspend();
            REQUIRE(num_update == 1);
            REQUIRE(num_cancel == 0);
        }
        WHEN("cancel") {
            timer.cancel();
            // give a short time to run the handler with queue
            std::this_thread::sleep_for(40ms); // give more than twice of start interval
            REQUIRE(num_update < 2);
            REQUIRE(num_cancel == 1);
        }
    }
    GIVEN("suspended") {
        timer.start(10ms, DISPATCH_TIME_NOW);
        std::this_thread::sleep_for(15ms);
        timer.suspend();
        REQUIRE(num_update == 2);
        REQUIRE(num_cancel == 0);
        WHEN("start again") {
            timer.start(20ms, DISPATCH_TIME_NOW);
            std::this_thread::sleep_for(10ms);
            timer.suspend();
            REQUIRE(num_update == 3);
        }
        WHEN("cancel") {
            // not resumed. so no cancel effect
            timer.cancel();
            std::this_thread::sleep_for(40ms); // give more than twice of start interval
            REQUIRE(num_update == 2);
            REQUIRE(num_cancel == 0);
        }
    }
}

struct resume_after_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

  public:
    resume_after_test_case() {
        REQUIRE(sem);
    }
    ~resume_after_test_case() {
        dispatch_release(sem);
    }

    static paused_action_t use_rvalue(dispatch_semaphore_t sem, dispatch_queue_t queue1, dispatch_queue_t queue2) {
        spdlog::debug("rvalue: {}", "before");
        co_await resume_after_t{queue1, 1s};
        spdlog::debug("rvalue: {}", "after 1s");
        co_await resume_after_t{queue2, 1s};
        spdlog::debug("rvalue: {}", "after 2s");
        dispatch_semaphore_signal(sem);
    }
    static paused_action_t use_lvalue(dispatch_semaphore_t sem, dispatch_queue_t queue) {
        resume_after_t awaiter{queue, 1s};
        spdlog::debug("lvalue: {}", "before");
        co_await awaiter;
        spdlog::debug("lvalue: {}", "after 1s");
        awaiter = 2s;
        co_await awaiter;
        spdlog::debug("lvalue: {}", "after 3s");
        dispatch_semaphore_signal(sem);
    }
};

TEST_CASE_METHOD(resume_after_test_case, "resume_after_t with single queue", "[dispatch][timer]") {
    paused_action_t action = use_lvalue(sem, queue);
    coroutine_handle<void> task = action.handle();
    task.resume();
    REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
    std::this_thread::sleep_for(1ms); // give short time to queue for task cleanup
    REQUIRE(task.done());
}

TEST_CASE_METHOD(resume_after_test_case, "resume_after_t with multiple queue", "[dispatch][timer]") {
    dispatch_queue_t queue_high = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    SECTION("increase priority") {
        paused_action_t action = use_rvalue(sem, queue, queue_high);
        coroutine_handle<void> task = action.handle();
        task.resume();
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        std::this_thread::sleep_for(1ms); // give short time to queue for task cleanup
        REQUIRE(task.done());
    }
    SECTION("decrease priority") {
        paused_action_t action = use_rvalue(sem, queue_high, queue);
        coroutine_handle<void> task = action.handle();
        task.resume();
        REQUIRE(dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0);
        std::this_thread::sleep_for(1ms); // give short time to queue for task cleanup
        REQUIRE(task.done());
    }
}

struct serial_queue_test_case {
    dispatch_queue_t queue = nullptr;
    const char* test_queue_label = "dev.luncliff.coro";

  public:
    serial_queue_test_case() {
        REQUIRE_FALSE(queue); // queue must be create in each TEST_CASE
    }
    ~serial_queue_test_case() {
        dispatch_release(queue);
    }

    static fire_and_forget run_once(dispatch_queue_t queue, std::mutex& mtx, std::vector<pthread_t>& records) {
        co_await queue_awaitable_t{queue};
        std::lock_guard lck{mtx};
        records.emplace_back(pthread_self());
    }

    static void validate(const std::vector<pthread_t>& records) {
        pthread_t prev_tid = 0;
        for (pthread_t tid : records) {
            if (prev_tid == 0) {
                prev_tid = tid;
                continue;
            }
            if (tid != prev_tid)
                FAIL("Detected different thread id");
        }
    }
};

/// @see https://stackoverflow.com/a/40635531
TEST_CASE_METHOD(serial_queue_test_case, "serial(autorelease)", "[dispatch][thread]") {
    dispatch_queue_attr_t attr = DISPATCH_QUEUE_SERIAL_WITH_AUTORELEASE_POOL;
    queue = dispatch_queue_create(test_queue_label, attr);
    REQUIRE(queue);

    std::vector<pthread_t> records{};
    std::mutex mtx{};
    auto repeat = 2000;
    while (repeat--)
        run_once(queue, mtx, records);
    // simply wait with time. the task is small so it won't take long
    std::this_thread::sleep_for(1000ms);
    REQUIRE(records.size() == 2000);
    validate(records);
}

/// @see https://stackoverflow.com/a/64286281
TEST_CASE_METHOD(serial_queue_test_case, "serial(target)", "[dispatch][thread]") {
    queue = dispatch_queue_create_with_target(test_queue_label, DISPATCH_QUEUE_SERIAL_WITH_AUTORELEASE_POOL,
                                              dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0));
    REQUIRE(queue);

    std::vector<pthread_t> records{};
    std::mutex mtx{};
    auto repeat = 2000;
    while (repeat--)
        run_once(queue, mtx, records);
    // simply wait with time. the task is small so it won't take long
    std::this_thread::sleep_for(1000ms);
    REQUIRE(records.size() == 2000);
    validate(records);
}

/// @see https://stackoverflow.com/a/40635531
TEST_CASE_METHOD(serial_queue_test_case, "serial(inactive)", "[dispatch][thread]") {
    queue = dispatch_queue_create(test_queue_label, dispatch_queue_attr_make_initially_inactive(DISPATCH_QUEUE_SERIAL));
    REQUIRE(queue);

    std::vector<pthread_t> records{};
    std::mutex mtx{};
    auto repeat = 100;
    while (repeat--)
        run_once(queue, mtx, records);

    std::this_thread::sleep_for(300ms);
    REQUIRE(records.size() == 0);

    dispatch_activate(queue); // now the tasks will be executed
    std::this_thread::sleep_for(300ms);
    REQUIRE(records.size() == 100); // the queue is drained
}

struct queue_owner_test_case {
    const char* test_queue_label = "dev.luncliff.coro";
    std::atomic_flag is_finalized = ATOMIC_FLAG_INIT;

    static void finalizer(void* ptr) {
        std::atomic_flag& flag = *reinterpret_cast<std::atomic_flag*>(ptr);
        if (flag.test_and_set() == true)
            spdlog::warn("the given atomic_flag is already set");
        // flag.notify_one(); // requires _LIBCPP_AVAILABILITY_SYNC
    }
    static fire_and_forget clear_flag(dispatch_queue_t queue, std::atomic_flag& flag) {
        co_await queue_awaitable_t{queue};
        flag.clear();
    }
};

TEST_CASE_METHOD(queue_owner_test_case, "queue finalize", "[dispatch][thread]") {
    {
        queue_owner_t queue{test_queue_label, DISPATCH_QUEUE_SERIAL, //
                            &is_finalized, &finalizer};
        clear_flag(queue.handle(), is_finalized); // ensure flag is cleared before the queue is released
    }
    // notice that the finalization is asynchronous. so we have to wait.
    // is_finalized.wait(true); // requires _LIBCPP_AVAILABILITY_SYNC
    std::this_thread::sleep_for(100ms);
    REQUIRE(is_finalized.test()); // marked true by the finalizer
}

#endif
