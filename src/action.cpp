#include "action.hpp"
#include "coro.hpp"

// #include <spdlog/spdlog.h>

namespace coro {

void sink_exception(std::exception_ptr&& exp) noexcept(false) {
    // spdlog::log(loc, spdlog::level::err, "{}", ex.what());
    std::rethrow_exception(exp);
}

void resume_once(void* ptr) noexcept {
    auto task = coroutine_handle<void>::from_address(ptr);
    if (task.done())
        return; // spdlog::warn("final-suspended coroutine_handle"); // probably because of the logic error
    task.resume();
}

coroutine_handle<void> end_awaitable_t::await_suspend(coroutine_handle<void>) noexcept {
    return handle;
}

void paused_action_t::promise_type::set_next(coroutine_handle<void> task) noexcept {
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

coroutine_handle<void>
paused_action_t::chained_awaitable_t::await_suspend(coroutine_handle<promise_type> coro) noexcept {
    action.promise().set_next(coro); // save the task so it can be continued
    return action;
}

suspend_never waitable_action_t::promise_type::initial_suspend() noexcept {
    if (proxy.retain)
        proxy.retain(proxy.context);
    return {};
}

suspend_always waitable_action_t::promise_type::final_suspend() noexcept {
    if (proxy.release)
        proxy.release(proxy.context);
    return {};
}

void waitable_action_t::promise_type::return_void() noexcept {
    return proxy.signal(proxy.context);
}

waitable_action_t::waitable_action_t(promise_type* p) noexcept : p{p} {
}

waitable_action_t::~waitable_action_t() noexcept {
    auto coro = coroutine_handle<promise_type>::from_promise(*p);
    coro.destroy();
}

void waitable_action_t::use(const event_proxy_t& e) noexcept(false) {
    event_proxy_t old = p->proxy;
    p->proxy = e;
    if (e.retain)
        e.retain(e.context);
    if (old.release)
        old.release(old.context);
}

uint32_t waitable_action_t::wait(uint32_t us) noexcept(false) {
    event_proxy_t& e = p->proxy;
    return e.wait(e.context, us);
}

} // namespace coro
