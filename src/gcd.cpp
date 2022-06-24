/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include "gcd.hpp"
#include <spdlog/spdlog.h>

namespace coro {

void sink_exception(const spdlog::source_loc& loc, std::exception_ptr&& exp) noexcept {
    try {
        std::rethrow_exception(exp);
    } catch (const std::exception& ex) {
        spdlog::log(loc, spdlog::level::err, ex.what());
    } catch (const std::error_code& ex) {
        spdlog::log(loc, spdlog::level::err, ex.message());
    } catch (const std::error_condition& ex) {
        spdlog::log(loc, spdlog::level::err, ex.message());
    } catch (...) {
        spdlog::log(loc, spdlog::level::critical, "unknwon exception type detected");
    }
}

void queue_awaitable_t::await_suspend(coroutine_handle<void> coro) noexcept {
    dispatch_async_f(queue, coro.address(), resume_once);
}

semaphore_owner_t::semaphore_owner_t() noexcept(false) : sem{dispatch_semaphore_create(0)} {
}

semaphore_owner_t::semaphore_owner_t(dispatch_semaphore_t handle) noexcept(false) : sem{handle} {
    if (handle == nullptr)
        throw std::invalid_argument{__func__};
    dispatch_retain(sem);
}

semaphore_owner_t::~semaphore_owner_t() noexcept {
    dispatch_release(sem);
}

semaphore_owner_t::semaphore_owner_t(const semaphore_owner_t& rhs) noexcept : semaphore_owner_t{rhs.sem} {
}

/// @note change the `promise_type`'s handle before `initial_suspend`
semaphore_action_t::semaphore_action_t(promise_type& p) noexcept
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

void semaphore_action_t::promise_type::unhandled_exception() noexcept {
    spdlog::source_loc loc{"semaphore_action_t", __LINE__, __func__};
    sink_exception(loc, std::current_exception());
}

void group_action_t::promise_type::unhandled_exception() noexcept {
    spdlog::source_loc loc{"group_action_t", __LINE__, __func__};
    sink_exception(loc, std::current_exception());
}

void timer_owner_t::set(void* context, dispatch_function_t on_event, dispatch_function_t on_cancel) noexcept {
    dispatch_set_context(source, context);
    dispatch_source_set_event_handler_f(source, on_event);
    dispatch_source_set_cancel_handler_f(source, on_cancel);
}

void timer_owner_t::start(std::chrono::nanoseconds interval, dispatch_time_t start) noexcept {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    dispatch_source_set_timer(source, start, interval.count(), duration_cast<nanoseconds>(500us).count());
    return dispatch_resume(source);
}

bool timer_owner_t::cancel(void* context, dispatch_function_t on_cancel) noexcept {
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

void timer_owner_t::suspend() noexcept {
    dispatch_suspend(source);
}

} // namespace coro
