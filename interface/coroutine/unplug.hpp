// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_UNPLUG_HPP
#define COROUTINE_UNPLUG_HPP

#include <experimental/coroutine>

// - Note
//      Commented code are examples for memory management customization.
// static std::allocator<char> __mgmt{};

// - Note
//      General `void` return for coroutine.
//      It doesn't provide any method to get flow/value
//      from the resumable function frame.
//
//      This type is an alternative of the `std::future<void>`
class unplug final
{
  public:
    struct promise_type
    {
        // No suspend for init/final suspension point
        auto initial_suspend() const noexcept
        {
            return std::experimental::suspend_never{};
        }
        auto final_suspend() const noexcept
        {
            return std::experimental::suspend_never{};
        }
        void return_void(void) noexcept {}
        void unhandled_exception() noexcept
        {
            // std::terminate();
        }

        promise_type& get_return_object() noexcept { return *this; }

        // - Note
        //      Examples for memory management customization.
        // void* operator new(size_t _size) noexcept(false)
        //{
        //    std::printf("%u \n", _size);
        //    return __mgmt.allocate(_size);
        //    return nullptr;
        //}

        // - Note
        //      Examples for memory management customization.
        // void operator delete(void* _ptr, size_t _size) noexcept
        //{
        //    std::printf("%u \n", _size);
        //    return __mgmt.deallocate(static_cast<char*>(_ptr), _size);
        //}
    };

  public:
    unplug(const promise_type&) noexcept {}
};

class await_plug final
{
    template<typename T = void>
    using handle_t = std::experimental::coroutine_handle<T>;

    handle_t<void> coro{};

  public:
    bool await_ready() noexcept { return false; }

    template<typename Promise>
    void await_suspend(handle_t<Promise> rh) noexcept
    {
        coro = rh;
    }
    void await_resume() noexcept
    {
        coro = nullptr; // forget
    }

  public:
    void resume() noexcept(false)
    {
        if (coro && coro.done() == false)
            // resume if available
            coro.resume();
    }
};

#endif // COROUTINE_UNPLUG_HPP
