// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Note
//      Coroutine utilities for this library
//
// ---------------------------------------------------------------------------
#ifndef _MAGIC_COROUTINE_HPP_
#define _MAGIC_COROUTINE_HPP_

#include <experimental/coroutine>
#include <experimental/generator>

namespace magic {
namespace stdex = std::experimental;

// - Note
//      Commented code are examples for memory management customization.
// extern std::allocator<char> __mgmt;

// - Note
//      General `void` return for coroutine.
//      It doesn't provide any method to get flow/value
//      from the resumable function frame.
//
//      This type is an alternative of the `std::future<void>`
class unplug {
public:
  struct promise_type {
    // No suspend for init/final suspension point
    auto initial_suspend() const noexcept { return stdex::suspend_never{}; }
    auto final_suspend() const noexcept { return stdex::suspend_never{}; }
    // Ignore return of the coroutine
    void return_void(void) noexcept {};

    promise_type &get_return_object() noexcept { return *this; }

    // - Note
    //      Examples for memory management customization.
    // void *operator new(size_t _size)
    //{
    //    return __mgmt.allocate(_size);
    //}

    // - Note
    //      Examples for memory management customization.
    // void operator delete(void *_ptr, size_t _size) noexcept
    //{
    //    return __mgmt.deallocate(static_cast<char *>(_ptr), _size);
    //}
  };

public:
  explicit unplug(const promise_type &) noexcept {};
};

} // namespace magic

#endif // _MAGIC_COROUTINE_HPP_
