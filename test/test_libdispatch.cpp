/**
 * @author  github.com/luncliff (luncliff@gmail.com)
 * @brief   Personal experiment with libdispatch in Apple platform.
 * 
 * @note    clang++ -std=c++2a -stdlib=libc++ -fcoroutines-ts
 * @see     https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/Introduction/Introduction.html
 * @see     https://apple.github.io/swift-corelibs-libdispatch/tutorial/
 */
#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>
// ...
#include <chrono>
#include <dispatch/dispatch.h>
#include <experimental/coroutine>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>

using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::experimental::coroutine_handle;
using std::experimental::noop_coroutine;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("%T %f [%l] %6t %v");
    Catch::Session session{};
    return session.run(argc, argv);
}

/**
 * @see C++ 20 <source_location>
 * 
 * @param loc Location hint to provide more information in the log
 * @param exp Exception caught in the coroutine's `unhandled_exception`
 */
void sink_exception(const spdlog::source_loc& loc, std::exception_ptr&& exp) noexcept {
    try {
        std::rethrow_exception(exp);
    } catch (const std::exception& ex) {
        spdlog::log(loc, spdlog::level::err, "{}", ex.what());
    } catch (...) {
        spdlog::critical("unknown exception type");
    }
}

/**
 * @details `__builtin_coro_resume` fits the signature, but we can't get an address of it
 *  because it's a compiler intrinsic. Create a simple function for the purpose.
 * 
 * @see dispatch_function_t
 */
void resume_once(void* ptr) noexcept {
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done())
        return spdlog::warn("final-suspended coroutine_handle"); // probably because of the logic error
    task.resume();
}

/**
 * @see C++/WinRT `winrt::fire_and_forget`
 */
struct fire_and_forget final {
    struct promise_type final {
        constexpr suspend_never initial_suspend() noexcept {
            return {};
        }
        constexpr suspend_never final_suspend() noexcept {
            return {};
        }
        void unhandled_exception() noexcept {
            // filename is useless. instead, use the coroutine return type's name
            spdlog::source_loc loc{"fire_and_forget", __LINE__, __func__};
            sink_exception(loc, std::current_exception());
        }
        constexpr void return_void() noexcept {
        }
        fire_and_forget get_return_object() noexcept {
            return fire_and_forget{*this};
        }
    };

    explicit fire_and_forget([[maybe_unused]] promise_type&) noexcept {
    }
};

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

class paused_action_t final {
  public:
    class promise_type final {
        coroutine_handle<void> next = nullptr; // task that should be continued after `final_suspend`

      public:
        constexpr suspend_always initial_suspend() noexcept {
            return {};
        }
        auto final_suspend() noexcept {
            struct awaitable_t final {
                coroutine_handle<void> handle;

              public:
                constexpr bool await_ready() noexcept {
                    return false;
                }
                constexpr coroutine_handle<void> await_suspend(coroutine_handle<void>) noexcept {
                    return handle;
                }
                constexpr void await_resume() noexcept {
                }
            };
            return awaitable_t{next ? next : noop_coroutine()};
        }
        void unhandled_exception() noexcept {
            spdlog::source_loc loc{"paused_action_t", __LINE__, __func__};
            sink_exception(loc, std::current_exception());
        }
        constexpr void return_void() noexcept {
        }
        paused_action_t get_return_object() noexcept {
            return paused_action_t{*this};
        }
        void set_next(coroutine_handle<void> task) noexcept {
            next = task;
        }
    };

  private:
    coroutine_handle<promise_type> coro;

  public:
    explicit paused_action_t(promise_type& p) noexcept : coro{coroutine_handle<promise_type>::from_promise(p)} {
    }
    ~paused_action_t() noexcept {
        if (coro)
            coro.destroy();
    }
    paused_action_t(const paused_action_t&) = delete;
    paused_action_t& operator=(const paused_action_t&) = delete;
    paused_action_t(paused_action_t&& rhs) noexcept : coro{rhs.coro} {
        rhs.coro = nullptr;
    }
    paused_action_t& operator=(paused_action_t&& rhs) noexcept {
        std::swap(coro, rhs.coro);
        return *this;
    }
    coroutine_handle<void> handle() const noexcept {
        return coro;
    }
    auto operator co_await() noexcept {
        struct awaitable_t final {
            coroutine_handle<promise_type> action; // handle of the `paused_action_t`

          public:
            constexpr bool await_ready() const noexcept {
                return false;
            }
            coroutine_handle<void> await_suspend(coroutine_handle<promise_type> coro) noexcept {
                action.promise().set_next(coro); // save the task so it can be continued
                return action;
            }
            constexpr void await_resume() const noexcept {
            }
        };
        // chain the next coroutine to given coroutine handle
        return awaitable_t{coro};
    }
};
static_assert(std::is_copy_constructible_v<paused_action_t> == false);
static_assert(std::is_copy_assignable_v<paused_action_t> == false);
static_assert(std::is_move_constructible_v<paused_action_t> == true);
static_assert(std::is_move_assignable_v<paused_action_t> == true);

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

/**
 * @brief Forward the `coroutine_handle`(job) to `dispatch_queue_t`
 * @see dispatch_async_f
 * @see https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/OperationQueues/OperationQueues.html
 */
struct queue_awaitable_t final {
    dispatch_queue_t queue;

  public:
    /// @brief true if `queue` is nullptr, resume immediately
    constexpr bool await_ready() const noexcept {
        return queue == nullptr;
    }
    /// @see dispatch_async_f
    void await_suspend(coroutine_handle<void> coro) noexcept {
        dispatch_async_f(queue, coro.address(), resume_once);
    }
    constexpr void await_resume() const noexcept {
    }
};
static_assert(std::is_nothrow_copy_constructible_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_copy_assignable_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_move_constructible_v<queue_awaitable_t> == true);
static_assert(std::is_nothrow_move_assignable_v<queue_awaitable_t> == true);

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

class semaphore_owner_t final {
    dispatch_semaphore_t sem;

  public:
    semaphore_owner_t() noexcept(false) : sem{dispatch_semaphore_create(0)} {
    }
    explicit semaphore_owner_t(dispatch_semaphore_t handle) noexcept(false) : sem{handle} {
        if (handle == nullptr)
            throw std::invalid_argument{__func__};
        dispatch_retain(sem);
    }
    ~semaphore_owner_t() noexcept {
        dispatch_release(sem);
    }
    semaphore_owner_t(const semaphore_owner_t& rhs) noexcept : semaphore_owner_t{rhs.sem} {
    }
    semaphore_owner_t(semaphore_owner_t&&) = delete;
    semaphore_owner_t& operator=(const semaphore_owner_t&) = delete;
    semaphore_owner_t& operator=(semaphore_owner_t&&) = delete;

    /// @todo provide a return value?
    bool signal() noexcept {
        return dispatch_semaphore_signal(sem);
    }
    [[nodiscard]] bool wait() noexcept {
        return dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER) == 0;
    }
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds duration) noexcept {
        return dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, duration.count())) == 0;
    }

    dispatch_semaphore_t handle() const noexcept {
        return sem;
    }
};
static_assert(std::is_copy_constructible_v<semaphore_owner_t> == true);
static_assert(std::is_copy_assignable_v<semaphore_owner_t> == false);
static_assert(std::is_move_constructible_v<semaphore_owner_t> == false);
static_assert(std::is_move_assignable_v<semaphore_owner_t> == false);

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

/**
 * @brief A coroutine return type which supports wait/wait_for with `dispatch_semaphore_t`
 * @details When this type is used, the coroutine's first argument must be a valid `dispatch_semaphore_t`
 */
class semaphore_action_t final {
  public:
    class promise_type final {
        friend class semaphore_action_t;

        dispatch_semaphore_t semaphore;

      public:
        /// @note This signature is for Clang compiler. defined(__clang__)
        template <typename... Args>
        promise_type(dispatch_semaphore_t handle, [[maybe_unused]] Args&&...) noexcept(false) : semaphore{handle} {
        }
        /// @note This signature is for MSVC. defined(_MSC_VER)
        promise_type() noexcept = default;
        promise_type(const promise_type&) = delete;
        promise_type(promise_type&&) = delete;
        promise_type& operator=(const promise_type&) = delete;
        promise_type& operator=(promise_type&&) = delete;

        suspend_never initial_suspend() noexcept {
            dispatch_retain(semaphore);
            return {};
        }
        suspend_never final_suspend() noexcept {
            dispatch_release(semaphore);
            return {};
        }
        void unhandled_exception() noexcept {
            spdlog::source_loc loc{"semaphore_action_t", __LINE__, __func__};
            sink_exception(loc, std::current_exception());
        }
        void return_void() noexcept {
            dispatch_semaphore_signal(semaphore);
        }
        semaphore_action_t get_return_object() noexcept {
            return semaphore_action_t{*this};
        }
    };

  private:
    semaphore_owner_t sem;

  private:
    /// @note change the `promise_type`'s handle before `initial_suspend`
    explicit semaphore_action_t(promise_type& p) noexcept
        // We have to share the semaphore between coroutine frame and return instance.
        // If `promise_type` is created with multiple arguments, the handle won't be nullptr.
        : sem{p.semaphore ? p.semaphore : dispatch_semaphore_create(0)} {
        // When the compiler default-constructed the `promise_type`, its semaphore will be nullptr.
        if (p.semaphore == nullptr) {
            // Share a newly created one
            p.semaphore = sem.handle();
            // We used dispatch_semaphore_create above.
            // The reference count is higher than what we want
            dispatch_release(p.semaphore);
        }
    }

  public:
    semaphore_action_t(const semaphore_action_t&) noexcept = default;
    semaphore_action_t(semaphore_action_t&&) noexcept = delete;
    semaphore_action_t& operator=(const semaphore_action_t&) noexcept = delete;
    semaphore_action_t& operator=(semaphore_action_t&&) noexcept = delete;

    [[nodiscard]] bool wait() noexcept {
        return sem.wait();
    }
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds duration) noexcept {
        return sem.wait_for(duration);
    }
};
static_assert(std::is_copy_constructible_v<semaphore_action_t> == true);
static_assert(std::is_copy_assignable_v<semaphore_action_t> == false);
static_assert(std::is_move_constructible_v<semaphore_action_t> == false);
static_assert(std::is_move_assignable_v<semaphore_action_t> == false);

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

/**
* @see dispatch_group_notify_f
*/
struct group_awaitable_t final {
    dispatch_group_t group;
    dispatch_queue_t queue;

  public:
    [[nodiscard]] bool await_ready() const noexcept {
        return dispatch_group_wait(group, DISPATCH_TIME_NOW) == 0;
    }
    /// @see dispatch_group_notify_f
    void await_suspend(coroutine_handle<void> coro) const noexcept {
        return dispatch_group_notify_f(group, queue, coro.address(), resume_once);
    }
    constexpr dispatch_group_t await_resume() const noexcept {
        return group;
    }
};

class group_action_t final {
  public:
    class promise_type final {
        dispatch_group_t group;

      public:
        // explicit promise_type(dispatch_group_t group) noexcept(false) : group{group} {
        //     if (group == nullptr)
        //         throw std::invalid_argument{__func__};
        //     dispatch_retain(group);
        // }
        template <typename... Args>
        promise_type(dispatch_group_t group, [[maybe_unused]] Args&&...) noexcept(false) : group{group} {
            if (group == nullptr)
                throw std::invalid_argument{__func__};
            dispatch_retain(group);
        }
        ~promise_type() {
            dispatch_release(group);
        }
        promise_type(const promise_type&) = delete;
        promise_type(promise_type&&) = delete;
        promise_type& operator=(const promise_type&) = delete;
        promise_type& operator=(promise_type&&) = delete;

        suspend_always initial_suspend() noexcept {
            dispatch_group_enter(group);
            return {};
        }
        suspend_never final_suspend() noexcept {
            dispatch_group_leave(group);
            return {};
        }
        void unhandled_exception() noexcept {
            spdlog::source_loc loc{"group_action_t", __LINE__, __func__};
            sink_exception(loc, std::current_exception());
        }
        constexpr void return_void() noexcept {
        }
        group_action_t get_return_object() noexcept {
            return group_action_t{*this, group};
        }
    };

  private:
    coroutine_handle<promise_type> coro;
    dispatch_group_t const group;

  public:
    /**
    * @note `dispatch_group_async_f` retains the group. so this type won't modify reference count of it
    * @see run_on
    */
    group_action_t(promise_type& p, dispatch_group_t group) noexcept
        : coro{coroutine_handle<promise_type>::from_promise(p)}, group{group} {
    }
    group_action_t(const group_action_t&) = delete;
    group_action_t(group_action_t&&) noexcept = default;
    group_action_t& operator=(const group_action_t&) = delete;
    group_action_t& operator=(group_action_t&&) = delete;

    /**
    * @brief Schedule the initial-suspended coroutine to the given queue
    * @note Using this function more than 1 will cause undefined behavior
    * @see dispatch_group_async_f
    * @param queue destination queue to schedule current coroutine handle
    */
    void run_on(dispatch_queue_t queue) noexcept {
        dispatch_group_async_f(group, queue, coro.address(), resume_once);
        coro = nullptr; // suspend_never in `promise_type::final_suspend`
    }
};
static_assert(std::is_copy_constructible_v<group_action_t> == false);
static_assert(std::is_copy_assignable_v<group_action_t> == false);
static_assert(std::is_move_constructible_v<group_action_t> == true);
static_assert(std::is_move_assignable_v<group_action_t> == false);

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

/**
* @brief Hold a timer and expose some shortcuts to start/suspend/cancel
* @see dispatch_source_create
* @see DISPATCH_SOURCE_TYPE_TIMER
*/
class timer_owner_t final {
    dispatch_source_t source;

  public:
    explicit timer_owner_t(dispatch_source_t timer) noexcept(false) : source{timer} {
        if (source == nullptr)
            throw std::runtime_error{"dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER)"};
        dispatch_retain(source);
    }
    explicit timer_owner_t(dispatch_queue_t queue) noexcept(false)
        : timer_owner_t{dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue)} {
    }
    ~timer_owner_t() noexcept {
        dispatch_release(source);
    }
    timer_owner_t(const timer_owner_t&) = delete;
    timer_owner_t& operator=(const timer_owner_t&) = delete;
    timer_owner_t(timer_owner_t&&) = delete;
    timer_owner_t& operator=(timer_owner_t&&) = delete;

    dispatch_source_t handle() const noexcept {
        return source;
    }
    void set(void* context, dispatch_function_t on_event, dispatch_function_t on_cancel) noexcept {
        dispatch_set_context(source, context);
        dispatch_source_set_event_handler_f(source, on_event);
        dispatch_source_set_cancel_handler_f(source, on_cancel);
    }
    void start(std::chrono::nanoseconds interval, dispatch_time_t start) noexcept {
        dispatch_source_set_timer(source, start, interval.count(), duration_cast<nanoseconds>(500us).count());
        return dispatch_resume(source);
    }
    /**
    * @param context   context of dispatch_source_t to change
    * @param on_cancel cancel handler of dispatch_source to change
    * @return true if the timer is canceled
    */
    bool cancel(void* context = nullptr, dispatch_function_t on_cancel = nullptr) noexcept {
        if (context)
            dispatch_set_context(source, context);
        if (on_cancel)
            dispatch_source_set_cancel_handler_f(source, on_cancel);
        // cancel after check
        if (dispatch_source_testcancel(source) != 0)
            return false;
        dispatch_source_cancel(source);
        return true;
    }
    void suspend() noexcept {
        dispatch_suspend(source);
    }
};

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

/**
* @brief Resume the given task in nanosecond interval
*/
class metronome_t final {
    timer_owner_t timer;
    coroutine_handle<void> task;

  private:
    static void resume_unfinished(coroutine_handle<void> task) noexcept {
        if (task.done())
            return;
        task.resume();
    }

  public:
    metronome_t(dispatch_queue_t queue, coroutine_handle<void> task) noexcept(false) : timer{queue}, task{task} {
        if (task.done())
            throw std::invalid_argument{"the task is already finished"};
    }

    void start(std::chrono::nanoseconds interval) noexcept {
        timer.set(task.address(), reinterpret_cast<dispatch_function_t>(resume_unfinished), nullptr);
        timer.start(interval, DISPATCH_TIME_NOW);
    }
    void cancel() noexcept {
        timer.cancel(timer.handle(), reinterpret_cast<dispatch_function_t>(dispatch_suspend));
    }
};

struct metronome_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  public:
    static paused_action_t signal_on_return(dispatch_semaphore_t sem, uint32_t repeat) {
        while (--repeat) {
            co_await suspend_always{};
            spdlog::debug("repeat: {}", repeat);
        }
        dispatch_semaphore_signal(sem);
    }
};

TEST_CASE_METHOD(metronome_test_case, "repeat with metronome_t", "[dispatch][timer]") {
    semaphore_owner_t sem{};
    paused_action_t action = signal_on_return(sem.handle(), 10);

    metronome_t metronome{queue, action.handle()};
    REQUIRE_NOTHROW(metronome.start(15ms));
    REQUIRE(sem.wait()); // DISPATCH_TIME_FOREVER
    metronome.cancel();

    // give short time to queue for task cleanup
    std::this_thread::sleep_for(1ms);
    REQUIRE(action.handle().done());
}

/**
* @brief Similar to `winrt::resume_after`, but works with `dispatch_queue_t`
* @see C++/WinRT `winrt::resume_after`
*/
struct resume_after_t final {
    dispatch_queue_t queue;
    std::chrono::nanoseconds duration;

  public:
    /// @brief true if `duration` is 0
    constexpr bool await_ready() const noexcept {
        return duration.count() == 0;
    }
    /// @see dispatch_after_f
    void await_suspend(coroutine_handle<void> coro) noexcept {
        const dispatch_time_t timepoint = dispatch_time(DISPATCH_TIME_NOW, duration.count());
        dispatch_after_f(timepoint, queue, coro.address(), resume_once);
    }
    constexpr void await_resume() const noexcept {
    }
    resume_after_t& operator=(std::chrono::nanoseconds duration) noexcept {
        this->duration = duration;
        return *this;
    }
};
static_assert(std::is_nothrow_copy_constructible_v<resume_after_t> == true);
static_assert(std::is_nothrow_copy_assignable_v<resume_after_t> == true);
static_assert(std::is_nothrow_move_constructible_v<resume_after_t> == true);
static_assert(std::is_nothrow_move_assignable_v<resume_after_t> == true);

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

class queue_owner_t {
    dispatch_queue_t queue = nullptr;

  public:
    queue_owner_t(const char* label, dispatch_queue_attr_t attr) : queue{dispatch_queue_create(label, attr)} {
    }
    queue_owner_t(const char* label, dispatch_queue_attr_t attr, void* context, dispatch_function_t on_finalize)
        : queue_owner_t{label, attr} {
        dispatch_set_context(queue, context);
        dispatch_set_finalizer_f(queue, on_finalize);
    }
    ~queue_owner_t() {
        dispatch_release(queue);
    }
    queue_owner_t(const queue_owner_t&) = delete;
    queue_owner_t(queue_owner_t&&) = delete;
    queue_owner_t& operator=(const queue_owner_t&) = delete;
    queue_owner_t& operator=(queue_owner_t&&) = delete;

    dispatch_queue_t handle() const noexcept {
        return queue;
    }
};
static_assert(std::is_copy_constructible_v<queue_owner_t> == false);
static_assert(std::is_copy_assignable_v<queue_owner_t> == false);
static_assert(std::is_move_constructible_v<queue_owner_t> == false);
static_assert(std::is_move_assignable_v<queue_owner_t> == false);

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

struct signal_source_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_source_t hook = nullptr;
    semaphore_owner_t sem{};

    using basic_handler_t = void (*)(int);
    basic_handler_t handler = nullptr;

    signal_source_test_case() = default;
    ~signal_source_test_case() {
        if (hook)
            dispatch_release(hook);
    }

    static void on_signal_event(signal_source_test_case* t) {
        dispatch_source_t source = t->hook;
        int signum = dispatch_source_get_handle(source);
        spdlog::debug("hooked signal: {}", signum);
        t->sem.signal();
    }

    static fire_and_forget raise_on_queue(dispatch_queue_t queue, int signum) {
        co_await queue_awaitable_t{queue};
        if (::raise(signum) == 0)
            spdlog::debug("raised signal: {}", signum);
        else
            spdlog::error("raised signal: {} {}", signum, errno);
    }
};

TEST_CASE_METHOD(signal_source_test_case, "raise signal(SIGHUP)", "[dispatch][signal]") {
    const auto signum = SIGHUP;
    // no exisiting handler. must be nullptr
    handler = ::signal(signum, SIG_IGN);
    REQUIRE(handler == nullptr);

    hook = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, signum, 0, queue);
    dispatch_set_context(hook, this);
    dispatch_source_set_event_handler_f(hook, reinterpret_cast<dispatch_function_t>(&on_signal_event));
    dispatch_resume(hook);

    raise_on_queue(queue, signum);
    spdlog::debug("waiting for raise routine to co_return");
    REQUIRE(sem.wait());
    dispatch_source_cancel(hook);
}

TEST_CASE_METHOD(signal_source_test_case, "raise signal(SIGPIPE)", "[dispatch][signal]") {
    const auto signum = SIGPIPE;
    // no exisiting handler. must be nullptr
    handler = ::signal(signum, SIG_IGN);
    REQUIRE(handler == nullptr);

    hook = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, signum, 0, queue);
    dispatch_set_context(hook, this);
    dispatch_source_set_event_handler_f(hook, reinterpret_cast<dispatch_function_t>(&on_signal_event));
    dispatch_resume(hook);

    raise_on_queue(queue, signum);
    spdlog::debug("waiting for raise routine to co_return");
    REQUIRE(sem.wait());
    dispatch_source_cancel(hook);
}

/// @note this TEST_CASE is not runnable with Xcode IDE
//TEST_CASE_METHOD(signal_source_test_case, "raise signal(SIGSEGV)", "[dispatch][signal]") {
//    const auto signum = SIGSEGV;
//    // core dump handler should be returned
//    handler = ::signal(signum, SIG_IGN);
//    REQUIRE(handler);
//
//    hook = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, signum, 0, queue);
//    dispatch_set_context(hook, this);
//    dispatch_source_set_event_handler_f(hook, reinterpret_cast<dispatch_function_t>(&on_signal_event));
//    dispatch_resume(hook);
//
//    raise_on_queue(queue, signum);
//    spdlog::debug("waiting for raise routine to co_return");
//    REQUIRE(sem.wait());
//    // rollback the handler changes
//    dispatch_source_cancel(hook);
//    REQUIRE(::signal(signum, handler) == SIG_IGN);
//}

class signal_hook_t {
    dispatch_source_t source;

  public:
    signal_hook_t(dispatch_queue_t queue, int signum) noexcept(false)
        : source{dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, signum, 0, queue)} {
        if (source == nullptr)
            throw std::runtime_error{"dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL)"};
    }
    ~signal_hook_t() noexcept {
        dispatch_release(source);
    }
    signal_hook_t(const signal_hook_t&) = delete;
    signal_hook_t(signal_hook_t&&) = delete;
    signal_hook_t& operator=(const signal_hook_t&) = delete;
    signal_hook_t& operator=(signal_hook_t&&) = delete;

    void resume(void* context, dispatch_function_t on_signal) noexcept {
        dispatch_set_context(source, context);
        dispatch_source_set_event_handler_f(source, on_signal);
        // add cancel handler for `dispatch_suspend`?
        dispatch_resume(source);
    }
    void cancel() noexcept {
        dispatch_source_cancel(source);
    }
    dispatch_source_t handle() const noexcept {
        return source;
    }
};
static_assert(std::is_copy_constructible_v<signal_hook_t> == false);
static_assert(std::is_copy_assignable_v<signal_hook_t> == false);
static_assert(std::is_move_constructible_v<signal_hook_t> == false);
static_assert(std::is_move_assignable_v<signal_hook_t> == false);

TEST_CASE("signal handler changes", "[signal]") {
    const auto signum = SIGUSR1;
    auto handler = ::signal(signum, SIG_IGN);
    REQUIRE(::signal(signum, handler) == SIG_IGN);
}

class handler_holder_t final {
  public:
    using type = void (*)(int);

  private:
    int signum = 0;
    type handler = nullptr;

  public:
    handler_holder_t(int signum, type in_use) noexcept : signum{signum}, handler{::signal(signum, in_use)} {
    }
    ~handler_holder_t() noexcept {
        ::signal(signum, handler);
    }
};
struct signal_hook_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    semaphore_owner_t sem{};
    dispatch_source_t source{};

    static void on_signal_event(signal_hook_test_case* t) {
        spdlog::debug("hooked signal: {}", static_cast<void*>(t->source));
        t->sem.signal();
    }

    static fire_and_forget raise_on_queue(dispatch_queue_t queue, int signum) {
        co_await queue_awaitable_t{queue};
        if (::raise(signum) != 0)
            spdlog::error("raise signal: {} {}", signum, errno);
    }
    static fire_and_forget raise_segv_on_queue(dispatch_queue_t queue, int signum) {
        co_await queue_awaitable_t{queue};
        spdlog::error("raising signal");
        int* ptr = nullptr;
        *ptr = 20;
        spdlog::error("raised signal");
    }
};

TEST_CASE_METHOD(signal_hook_test_case, "hook signal(SIGUSR1)", "[dispatch][signal]") {
    const auto signum = SIGUSR1;
    handler_holder_t holder{signum, SIG_IGN};

    signal_hook_t hook{queue, signum};
    this->source = hook.handle();
    hook.resume(static_cast<signal_hook_test_case*>(this), reinterpret_cast<dispatch_function_t>(&on_signal_event));

    raise_on_queue(queue, signum);
    REQUIRE(sem.wait());
    hook.cancel();
}

SCENARIO_METHOD(signal_hook_test_case, "hook signal multiple times", "[dispatch][signal]") {
    GIVEN("signal_hook_t(SIGUSR1/SIG_IGN)") {
        const auto signum = SIGUSR1;
        handler_holder_t holder{signum, SIG_IGN};

        signal_hook_t hook{queue, signum};
        this->source = hook.handle();
        hook.resume(static_cast<signal_hook_test_case*>(this), reinterpret_cast<dispatch_function_t>(&on_signal_event));

        WHEN("raise 2 and cancel") {
            raise_on_queue(queue, signum);
            REQUIRE(sem.wait());
            raise_on_queue(queue, signum);
            REQUIRE(sem.wait());
            hook.cancel();
        }
    }
}

TEST_CASE_METHOD(signal_hook_test_case, "hook signal(SIGSEGV)", "[dispatch][signal][!mayfail]") {
    FAIL("SIGSEGV can't be hooked");
    const auto signum = SIGSEGV;
    handler_holder_t holder{signum, SIG_IGN};

    signal_hook_t hook{queue, signum};
    this->source = hook.handle();
    hook.resume(static_cast<signal_hook_test_case*>(this), reinterpret_cast<dispatch_function_t>(&on_signal_event));

    raise_segv_on_queue(queue, signum);
    REQUIRE_FALSE(sem.wait_for(100ms));
    hook.cancel();
}

TEST_CASE_METHOD(signal_hook_test_case, "hook signal(SIGUSR2)", "[dispatch][signal]") {
    const auto signum = SIGUSR2;
    handler_holder_t holder{signum, SIG_IGN};

    signal_hook_t hook{queue, signum};
    this->source = hook.handle();
    hook.resume(static_cast<signal_hook_test_case*>(this), reinterpret_cast<dispatch_function_t>(&on_signal_event));

    raise_on_queue(queue, signum);
    REQUIRE(sem.wait());
    hook.cancel();
}

struct fd_test_case {
    int fd[2]{}; // [inbound, outbound]

    fd_test_case() {
        if (pipe(fd) == 0) // non-zero if failed
            return;
        const auto ec = errno;
        FAIL(ec);
    }
    ~fd_test_case() {
        close(fd[0]);
        close(fd[1]);
    }
};

struct io_source_test_case : public fd_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_source_t reader = nullptr;
    size_t rsz = 0;
    size_t rcount = 0;
    dispatch_source_t writer = nullptr;
    size_t wsz = 0;
    size_t wcount = 0;

    static void on_read_available(io_source_test_case* t) {
        dispatch_source_t source = t->reader;
        ++t->rcount;
        const int fd = dispatch_source_get_handle(source);
        const size_t available = dispatch_source_get_data(source);
        std::string message{};
        message.resize(available);
        iovec bufs[1]{{message.data(), message.length()}};
        const auto len = ::readv(fd, bufs, 1);
        if (len == -1) {
            spdlog::error("read: {} {}", fd, errno);
            return dispatch_source_cancel(source);
        }
        t->rsz += len;
        if (len == 0) { // EOF
            spdlog::debug("read: {} {}", fd, t->rsz);
            return dispatch_source_cancel(source);
        }
    }

    static void on_reader_cancel(io_source_test_case* t) {
        dispatch_source_t source = t->reader;
        const int fd = dispatch_source_get_handle(source);
        spdlog::debug("cancel: {}", fd);
        dispatch_release(source);
    }

    static void on_write_available(io_source_test_case* t) {
        dispatch_source_t source = t->writer;
        ++t->wcount;
        const int fd = dispatch_source_get_handle(source);
        std::string message = __func__;
        iovec bufs[1]{{message.data(), message.length()}};
        const auto len = ::writev(fd, bufs, 1);
        if (len == -1) {
            spdlog::error("write: {} {}", fd, errno);
            return dispatch_source_cancel(source);
        }
        t->wsz += len;
        if (t->wsz > 2000) { // enough. stop the send
            spdlog::debug("write: {} {}", fd, t->wsz);
            dispatch_source_cancel(source);
        }
    }

    static void on_writer_cancel(io_source_test_case* t) {
        dispatch_source_t source = t->writer;
        int fd = dispatch_source_get_handle(source);
        spdlog::debug("cancel: {}", fd);
        dispatch_release(source);
    }
};

SCENARIO_METHOD(io_source_test_case, "dispatch reader suspend/resume", "[dispatch][io]") {
    reader = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd[0], 0, queue);
    dispatch_set_context(reader, this);
    dispatch_source_set_event_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_read_available));
    GIVEN("suspended") {
        WHEN("suspend 2 times") {
            dispatch_suspend(reader);
            dispatch_suspend(reader);
        }
        WHEN("resume/suspend 2 times") {
            dispatch_resume(reader);
            dispatch_suspend(reader);
            dispatch_resume(reader);
            dispatch_suspend(reader);
        }
    }
    GIVEN("resumed") {
        dispatch_resume(reader);
        WHEN("suspend 2 times") {
            dispatch_suspend(reader);
            dispatch_suspend(reader);
        }
        WHEN("resume/suspend 2 times") {
            // dispatch_resume(reader); // raise SIGILL
            dispatch_suspend(reader);
            dispatch_resume(reader);
            dispatch_suspend(reader);
        }
    }
}

SCENARIO_METHOD(io_source_test_case, "dispatch writer suspend/resume", "[dispatch][io]") {
    writer = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, fd[1], 0, queue);
    dispatch_set_context(writer, this);
    dispatch_source_set_event_handler_f(writer, reinterpret_cast<dispatch_function_t>(&on_write_available));
    GIVEN("suspended") {
        WHEN("suspend 2 times") {
            dispatch_suspend(writer);
            dispatch_suspend(writer);
        }
        WHEN("resume/suspend 2 times") {
            dispatch_resume(writer);
            dispatch_suspend(writer);
            dispatch_resume(writer);
            dispatch_suspend(writer);
        }
    }
    GIVEN("resumed") {
        dispatch_resume(writer);
        WHEN("suspend 2 times") {
            dispatch_suspend(writer);
            dispatch_suspend(writer);
        }
        WHEN("resume/suspend 2 times") {
            // dispatch_resume(writer); // raise SIGILL
            dispatch_suspend(writer);
            dispatch_resume(writer);
            dispatch_suspend(writer);
        }
    }
}

TEST_CASE_METHOD(io_source_test_case, "dispatch reader", "[dispatch][io]") {
    reader = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd[0], 0, queue);
    dispatch_set_context(reader, this);
    dispatch_source_set_event_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_read_available));
    dispatch_source_set_cancel_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_reader_cancel));
    dispatch_resume(reader);

    std::string message = __func__;
    iovec bufs[1]{{message.data(), message.length()}};
    const auto len = ::writev(fd[1], bufs, 1);
    if (auto ec = errno; len < 0)
        FAIL(ec);

    std::this_thread::sleep_for(1s);
    REQUIRE(rsz == message.length());

    dispatch_source_cancel(reader);
    std::this_thread::sleep_for(100ms);
    dispatch_suspend(reader);
}

struct io_source_with_signal_test_case : public io_source_test_case {
    signal_hook_t hook{queue, SIGPIPE};

    io_source_with_signal_test_case() : io_source_test_case{} {
        hook.resume(this, reinterpret_cast<dispatch_function_t>(&on_signal_event));
    }
    ~io_source_with_signal_test_case() {
        hook.cancel();
    }
    static void on_signal_event(io_source_with_signal_test_case* t) {
        dispatch_source_t source = t->hook.handle();
        int signum = dispatch_source_get_handle(source);
        spdlog::warn("signal: {}", signum);
    }
};

TEST_CASE_METHOD(io_source_with_signal_test_case, "dispatch reader when fd closed", "[dispatch][io][!mayfail]") {
    reader = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd[0], 0, queue);
    dispatch_set_context(reader, this);
    dispatch_source_set_event_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_read_available));
    dispatch_source_set_cancel_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_reader_cancel));

    close(fd[0]); // leads to signal SIGPIPE(13). consumed by the hook.
    dispatch_resume(reader);
    std::this_thread::sleep_for(1s);

    REQUIRE(rcount == 0);
    REQUIRE(rsz == 0);
    dispatch_source_cancel(reader);
    // give enough time to cancel handlers so they can return before the destruction of this TEST_CASE
    std::this_thread::sleep_for(200ms);
}

TEST_CASE_METHOD(io_source_test_case, "dispatch writer when fd closed", "[dispatch][io]") {
    writer = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, fd[1], 0, queue);
    dispatch_set_context(writer, this);
    dispatch_source_set_event_handler_f(writer, reinterpret_cast<dispatch_function_t>(&on_write_available));
    dispatch_source_set_cancel_handler_f(writer, reinterpret_cast<dispatch_function_t>(&on_writer_cancel));

    close(fd[1]); // close the file before the writer starts
    dispatch_resume(writer);
    std::this_thread::sleep_for(1s);
    REQUIRE(wcount == 0); // event handler didn't invoked
    REQUIRE(wsz == 0);

    dispatch_source_cancel(writer);
    std::this_thread::sleep_for(100ms);
    dispatch_suspend(writer);
}

TEST_CASE_METHOD(io_source_test_case, "dispatch fd reader/writer", "[dispatch][io]") {
    reader = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd[0], 0, queue);
    dispatch_set_context(reader, this);
    dispatch_source_set_event_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_read_available));
    dispatch_source_set_cancel_handler_f(reader, reinterpret_cast<dispatch_function_t>(&on_reader_cancel));
    dispatch_resume(reader);

    writer = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, fd[1], 0, queue);
    dispatch_set_context(writer, this);
    dispatch_source_set_event_handler_f(writer, reinterpret_cast<dispatch_function_t>(&on_write_available));
    dispatch_source_set_cancel_handler_f(writer, reinterpret_cast<dispatch_function_t>(&on_writer_cancel));
    dispatch_resume(writer);

    // recv/send is in-progress. we don't exchange that much data
    // so this must be enough time to finish recv/send/cancel
    std::this_thread::sleep_for(1s);
    REQUIRE(rsz == wsz);
    // we are using pipe. it won't detect EOF with this code
    // trigger cancel and wait for the async operation
    dispatch_source_cancel(reader);
    // close(fd[1]); // widow the pipe reader so it can be canceled.

    // give enough time to cancel handlers so they can return before the destruction of this TEST_CASE
    std::this_thread::sleep_for(200ms);
}

class io_read_hook_t final {
    dispatch_source_t reader;

  public:
    io_read_hook_t(dispatch_queue_t queue, int fd) noexcept(false)
        : reader{dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd, 0, queue)} {
        if (reader == nullptr)
            throw std::runtime_error{"dispatch_source_create(DISPATCH_SOURCE_TYPE_READ)"};
        dispatch_retain(reader);
    }
    ~io_read_hook_t() {
        dispatch_release(reader);
    }
    io_read_hook_t(const io_read_hook_t&) = delete;
    io_read_hook_t(io_read_hook_t&&) = delete;
    io_read_hook_t& operator=(const io_read_hook_t&) = delete;
    io_read_hook_t& operator=(io_read_hook_t&&) = delete;

    /// @brief always trigger await_suspend
    constexpr bool await_ready() const noexcept {
        return false;
    }

    /**
     * @note `reader` will be suspended when this coroutine is resumed 
     */
    void await_suspend(coroutine_handle<void> coro) noexcept {
        dispatch_set_context(reader, coro.address());
        dispatch_source_set_event_handler_f(reader, resume_once);
        dispatch_resume(reader);
    }

    /**
     * @note `reader` will be resumed when next read is requested
     * @return size_t available bytes. 
     * @see `dispatch_source_get_data`
     */
    [[nodiscard]] size_t await_resume() noexcept {
        dispatch_suspend(reader);
        const size_t available = dispatch_source_get_data(reader);
        return available;
    }

    int fileno() const noexcept {
        const int fd = dispatch_source_get_handle(reader);
        return fd;
    }
};

class io_write_hook_t final {
    dispatch_source_t writer;

  public:
    io_write_hook_t(dispatch_queue_t queue, int fd) noexcept(false)
        : writer{dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, fd, 0, queue)} {
        if (writer == nullptr)
            throw std::runtime_error{"dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE)"};
        dispatch_retain(writer);
    }
    ~io_write_hook_t() {
        dispatch_release(writer);
    }
    io_write_hook_t(const io_write_hook_t&) = delete;
    io_write_hook_t(io_write_hook_t&&) = delete;
    io_write_hook_t& operator=(const io_write_hook_t&) = delete;
    io_write_hook_t& operator=(io_write_hook_t&&) = delete;

    /// @brief always trigger await_suspend
    constexpr bool await_ready() const noexcept {
        return false;
    }

    /**
     * @note `writer` will be suspended when this coroutine is resumed 
     */
    void await_suspend(coroutine_handle<void> coro) noexcept {
        dispatch_set_context(writer, coro.address());
        dispatch_source_set_event_handler_f(writer, resume_once);
        dispatch_resume(writer);
    }

    /**
     * @note `writer` will be resumed when next read is requested
     */
    void await_resume() noexcept {
        dispatch_suspend(writer);
    }

    int fileno() const noexcept {
        const int fd = dispatch_source_get_handle(writer);
        return fd;
    }
};

struct io_hook_test_case : public fd_test_case {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    static semaphore_action_t write_some(dispatch_semaphore_t semaphore, dispatch_queue_t queue, int fd,
                                         std::string message) {
        co_await suspend_never{};
        io_write_hook_t hook{queue, fd};
        co_await hook;
        auto len = write(fd, message.data(), message.length());
        spdlog::debug("write: {} {}", len, errno);
    }
    static semaphore_action_t read_some(dispatch_semaphore_t semaphore, dispatch_queue_t queue, int fd,
                                        std::string& message) {
        co_await suspend_never{};
        io_read_hook_t hook{queue, fd};
        const size_t available = co_await hook; // register coroutine, resume, suspend
        message.resize(available);
        auto len = read(fd, message.data(), message.length());
        spdlog::debug("read: {} {}", len, errno);
    }
};

TEST_CASE_METHOD(io_hook_test_case, "DISPATCH_SOURCE_TYPE_READ release", "[dispatch][io][lifecycle]") {
    io_read_hook_t hook{queue, fd[0]};
    REQUIRE_FALSE(hook.await_ready());
    REQUIRE(hook.fileno() == fd[0]);
}
TEST_CASE_METHOD(io_hook_test_case, "DISPATCH_SOURCE_TYPE_WRITE release", "[dispatch][io][lifecycle]") {
    io_write_hook_t hook{queue, fd[1]};
    REQUIRE_FALSE(hook.await_ready());
    REQUIRE(hook.fileno() == fd[1]);
}

TEST_CASE_METHOD(io_hook_test_case, "io_read_hook_t read", "[dispatch][io]") {
    std::string message{};
    semaphore_action_t action = read_some(dispatch_semaphore_create(0), queue, fd[0], message);
    {
        const char* txt = "qqwertyuiopasdfghjklzxcvbnm1234567890";
        iovec outbound{};
        outbound.iov_base = const_cast<char*>(txt);
        outbound.iov_len = strnlen(txt, 40);
        REQUIRE(writev(fd[1], &outbound, 1) > 0);
    }
    REQUIRE(action.wait());
    REQUIRE(message.length() > 0);
}

TEST_CASE_METHOD(io_hook_test_case, "io_read_hook_t write", "[dispatch][io]") {
    semaphore_action_t action = write_some(dispatch_semaphore_create(0), queue, //
                                           fd[1], "qqwertyuiopasdfghjklzxcvbnm1234567890");
    std::string message{};
    message.resize(40);
    {
        iovec outbound{};
        outbound.iov_base = message.data();
        outbound.iov_len = message.length();
        REQUIRE(readv(fd[0], &outbound, 1) > 0);
    }
    REQUIRE(action.wait());
}
