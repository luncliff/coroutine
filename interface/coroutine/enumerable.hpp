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

#ifdef __clang__

template<typename T>
class enumerable final
{
  public:
    struct promise;
    using promise_type = attach_u64_type_size<promise>;

    class iterator;

    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

  private:
    // alias resumable handle
    using handle_promise_t = std::experimental::coroutine_handle<promise_type>;
    handle_promise_t rh;

  private: // disable copy / move for safe usage
    enumerable(const enumerable&) = delete;
    enumerable& operator=(const enumerable&) = delete;
    enumerable(enumerable&&) = delete;
    enumerable& operator=(enumerable&&) = delete;

  public:
    enumerable(promise* p) noexcept : rh{nullptr}
    {
        auto ptr = reinterpret_cast<promise_type*>(p);
        this->rh = handle_promise_t::from_promise(*ptr);
        // must ensure handle is valid...
        assert(rh.address() != nullptr);
    }
    ~enumerable() noexcept
    {
        // enumerable will destroy the frame.
        // so sub-types aren't need to consider about it
        if (rh) rh.destroy();
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
    struct promise // Resumable Promise Requirement
    {
        friend class iterator;

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
        void return_void() noexcept { current = nullptr; }
        void unhandled_exception() noexcept { std::terminate(); }

        promise* get_return_object() noexcept
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
        handle_promise_t rh;

      public:
        // `enumerable::end()`
        iterator(nullptr_t) noexcept : rh{nullptr} {}
        // `enumerable::begin()`
        iterator(handle_promise_t handle) noexcept : rh{handle} {}

      public:
        iterator& operator++() noexcept(false)
        {
            rh.resume();
            if (rh.done())
                // enumerable will destroy this frame later...
                rh = nullptr;
            return *this;
        }

        // post increment
        iterator& operator++(int) = delete;

        decltype(auto) operator*() noexcept
        {
            return *(rh.promise().current); //
        }
        decltype(auto) operator*() const noexcept
        {
            return *(rh.promise().current); //
        }

        decltype(auto) operator-> () noexcept
        {
            return rh.promise().current; //
        }
        decltype(auto) operator-> () const noexcept
        {
            return rh.promise().current; //
        }

        bool operator==(const iterator& rhs) const noexcept
        {
            return this->rh == rhs.rh;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };
};

#elif _MSC_VER // clang compiler will use the followings...

#include <experimental/generator>

template<typename T, typename A = std::allocator<uint64_t>>
using enumerable = std::experimental::generator<T, A>;

#endif // _MSC_VER

#endif // COROUTINE_ENUMERABLE_HPP
