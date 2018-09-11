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
#ifdef _WIN32
#include <experimental/generator>
#else

namespace std
{
namespace experimental
{
// - Note
//    This implementation should work like <experimental/generator> of MSVC
template <typename T,
          typename A = allocator<char>> // byte level allocation by default
struct generator
{
  struct promise_type; // Resumable Promise Requirement
  struct iterator;     // Abstraction: iterator

private:
  coroutine_handle<promise_type> rh{}; // resumable handle

public:
  generator(coroutine_handle<promise_type> &&handle_from_promise) noexcept
      : rh{std::move(handle_from_promise)}
  {
    // must ensure handle is valid...
  }

  // copy
  generator(const generator &) = delete;
  // move
  generator(generator &&rhs) : rh(std::move(rhs.rh)) {}

  ~generator() noexcept
  {
    // generator will destroy the frame.
    // so sub-types aren't need to consider about it
    if (rh)
      rh.destroy();
  }

public:
  iterator begin() noexcept(false)
  {
    if (rh) // resumeable?
    {
      rh.resume();
      if (rh.done()) // finished?
        return nullptr;
    }
    return iterator{rh};
  }
  iterator end() noexcept { return iterator{nullptr}; }

public:
  // Resumable Promise Requirement
  struct promise_type
  {
  public:
    // iterator will access to this pointer
    //  and reference memory object in coroutine frame
    T *current = nullptr;

  private:
    // promise type is strongly coupled to coroutine frame.
    // so copy/move might create wrong(garbage) coroutine handle
    promise_type(promise_type &) noexcept = delete;
    promise_type(promise_type &&) = delete;

  public:
    promise_type() = default;
    ~promise_type() = default;

  public:
    // suspend at init/final suspension point
    auto initial_suspend() const noexcept { return suspend_always{}; }
    auto final_suspend() const noexcept { return suspend_always{}; }
    // `co_yield` expression. only for reference
    auto yield_value(T &ref) noexcept
    {
      current = std::addressof(ref);
      return suspend_always{};
    }
    // `co_yield` expression. for value
    // auto yield_value(T &&value) noexcept
    // {
    //   cache = std::move(value)
    //   current = std::addressof(cache);
    //   return suspend_always{};
    // }

    // `co_return` expression
    void return_void() noexcept { current = nullptr; }
    // terminate if unhandled exception occurs
    void unhandled_exception() noexcept { std::terminate(); }

    coroutine_handle<promise_type> get_return_object() noexcept
    {
      // generator will hold this handle
      return coroutine_handle<promise_type>::from_promise(*this);
    }
  };

  // Abstraction: iterator
  struct iterator
      : public std::iterator<std::input_iterator_tag, T>
  {
    coroutine_handle<promise_type> rh;

  public:
    // `generator::end()`
    iterator(nullptr_t) noexcept : rh{nullptr} {}
    // `generator::begin()`
    iterator(coroutine_handle<promise_type> handle) noexcept : rh{handle} {}

  public:
    iterator &operator++() noexcept(false)
    {
      rh.resume();
      // generator will destroy coroutine frame later...
      if (rh.done())
        rh = nullptr;
      return *this;
    }

    // post increment
    iterator &operator++(int) = delete;

    auto operator*() noexcept -> T & { return *(rh.promise().current); }
    auto operator*() const noexcept -> const T & { return *(rh.promise().current); }

    auto operator-> () noexcept -> T * { return rh.promise().current; }
    auto operator-> () const noexcept -> const T * { return rh.promise().current; }

    bool operator==(const iterator &rhs) const noexcept { return this->rh == rhs.rh; }
    bool operator!=(const iterator &rhs) const noexcept { return !(*this == rhs); }
  };
};

} // namespace experimental
} // namespace std

#endif // _WIN32 <experimental/generator>

namespace magic
{
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
class unplug
{
public:
  struct promise_type
  {
    // No suspend for init/final suspension point
    auto initial_suspend() const noexcept { return stdex::suspend_never{}; }
    auto final_suspend() const noexcept { return stdex::suspend_never{}; }
    // Ignore return of the coroutine
    void return_void(void) noexcept {}
    void unhandled_exception() noexcept {}

    promise_type &get_return_object() noexcept { return *this; }

    // - Note
    //      Examples for memory management customization.
    // void *operator new(size_t _size) noexcept(false)
    // {
    //   //  return __mgmt.allocate(_size);
    //   return nullptr;
    // }

    // - Note
    //      Examples for memory management customization.
    // void operator delete(void *_ptr, size_t _size) noexcept
    // {
    //   //  return __mgmt.deallocate(static_cast<char *>(_ptr), _size);
    // }
  };

public:
  unplug(const promise_type &) noexcept {}
};

} // namespace magic

#endif // _MAGIC_COROUTINE_HPP_
