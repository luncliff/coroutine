// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      The alias of `generator`. Since the name is a kind of reserved one,
//      we will alias it to `enumerable`
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_ENUMERABLE_HPP
#define COROUTINE_ENUMERABLE_HPP

#include <coroutine/frame.h>
#include <iterator>

// - Note
//      Another implementation of <experimental/generator>
template <typename T>
class enumerable final
{
  public:
    struct promise_type;
    class iterator;

    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

  private:
    // alias resumable handle types

    using handle_t = std::experimental::coroutine_handle<void>;
    using handle_promise_t = std::experimental::coroutine_handle<promise_type>;

    handle_promise_t coro;

  private: // disable copy / move for safe usage
    enumerable(const enumerable&) = delete;
    enumerable& operator=(const enumerable&) = delete;
    enumerable(enumerable&&) = delete;
    enumerable& operator=(enumerable&&) = delete;

  public:
    enumerable(promise_type* ptr) noexcept
        : coro{handle_promise_t::from_promise(*ptr)}
    {
    }

    ~enumerable() noexcept
    {
        // enumerable will destroy the frame.
        if (coro) // therefore promise/iterator are free from those ownership
            coro.destroy();
    }

  public:
    iterator begin() noexcept(false)
    {
        if (coro) // resumeable?
        {
            coro.resume();
            if (coro.done()) // finished?
                return iterator{nullptr};
        }
        return iterator{coro};
    }
    iterator end() noexcept
    {
        return iterator{nullptr};
    }

  public:
    class promise_type final // Resumable Promise Requirement
    {
        friend class iterator;
        friend class enumerable;

        pointer current = nullptr;

      public:
        auto initial_suspend() const noexcept
        {
            return std::experimental::suspend_always{};
        }
        auto final_suspend() const noexcept
        {
            return std::experimental::suspend_always{};
        }

        // `co_yield` expression. only for reference
        auto yield_value(reference ref) noexcept
        {
            current = std::addressof(ref);
            return std::experimental::suspend_always{};
        }

        // `co_return` expression
        void return_void() noexcept
        {
            // no more access to value
            current = nullptr;
        }
        void unhandled_exception() noexcept
        {
            // just terminate?
            std::terminate();
        }

        promise_type* get_return_object() noexcept
        {
            // enumerable will create coroutine handle from the address
            return this;
        }
    };

    class iterator final
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = T;
        using reference = T&;
        using pointer = T*;

      public:
        handle_promise_t coro; // resumable handle

      public:
        // `enumerable::end()`
        explicit iterator(std::nullptr_t) noexcept : coro{nullptr}
        {
        }
        // `enumerable::begin()`
        explicit iterator(handle_promise_t handle) noexcept : coro{handle}
        {
        }

      public:
        iterator& operator++(int) = delete; // post increment
        iterator& operator++() noexcept(false)
        {
            coro.resume();
            if (coro.done())    // enumerable will destroy
                coro = nullptr; // the frame later...

            return *this;
        }

        pointer operator->() noexcept
        {
            pointer ptr = coro.promise().current;
            return ptr;
        }
        pointer operator->() const noexcept
        {
            pointer ptr = coro.promise().current;
            return ptr;
        }
        reference operator*() noexcept
        {
            return *(this->operator->());
        }
        reference operator*() const noexcept
        {
            return *(this->operator->());
        }

        bool operator==(const iterator& rhs) const noexcept
        {
            return this->coro == rhs.coro;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };
};

#endif // COROUTINE_ENUMERABLE_HPP
