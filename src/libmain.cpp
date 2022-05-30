/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
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

} // namespace coro
