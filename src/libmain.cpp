/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <chrono>
#include <coroutine/action.hpp>
#include <spdlog/spdlog.h>

namespace coro {

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

void fire_and_forget::promise_type::unhandled_exception() noexcept {
    // filename is useless. instead, use the coroutine return type's name
    spdlog::source_loc loc{"fire_and_forget", __LINE__, __func__};
    sink_exception(loc, std::current_exception());
}

void paused_promise_t::unhandled_exception() noexcept {
    spdlog::source_loc loc{"paused_action_t", __LINE__, __func__};
    sink_exception(loc, std::current_exception());
}

void paused_promise_t::set_next(coroutine_handle<void> task) noexcept {
    next = task;
}

paused_action_t::paused_action_t(promise_type& p) noexcept : coro{coroutine_handle<promise_type>::from_promise(p)} {
}

paused_action_t::~paused_action_t() noexcept {
    if (coro)
        coro.destroy();
}

paused_action_t::paused_action_t(paused_action_t&& rhs) noexcept : coro{rhs.coro} {
    rhs.coro = nullptr;
}

paused_action_t& paused_action_t::operator=(paused_action_t&& rhs) noexcept {
    std::swap(coro, rhs.coro);
    return *this;
}

coroutine_handle<void> paused_action_t::handle() const noexcept {
    return coro;
}

paused_action_t paused_promise_t::get_return_object() noexcept {
    return paused_action_t{*this};
}

void resume_once(void* ptr) noexcept {
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done())
        return spdlog::warn("final-suspended coroutine_handle"); // probably because of the logic error
    task.resume();
}

#if __has_include(<dispatch/dispatch.h>)

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

#endif

} // namespace coro
