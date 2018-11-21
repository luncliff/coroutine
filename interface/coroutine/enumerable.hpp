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
template<typename T>
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

    handle_t coro;

  private: // disable copy / move for safe usage
    enumerable(const enumerable&) = delete;
    enumerable& operator=(const enumerable&) = delete;
    enumerable(enumerable&&) = delete;
    enumerable& operator=(enumerable&&) = delete;

  public:
    enumerable(promise_type* ptr) noexcept : coro{nullptr}
    {
        if constexpr (is_clang)
        {
            // calculate the location of the coroutine frame prefix
            void* prefix =
                reinterpret_cast<char*>(ptr) - sizeof(clang_frame_prefix);
            coro = handle_t::from_address(prefix);
        }
        else if constexpr (is_msvc)
        {
            coro = handle_promise_t::from_promise(*ptr);
        }
        else
        {
            coro = handle_t::from_address(nullptr);
        }

        assert(coro.address() != nullptr); // must ensure handle is valid...
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
                return nullptr;
        }
        return iterator{coro};
    }
    iterator end() noexcept { return iterator{nullptr}; }

  public:
    struct promise_type // Resumable Promise Requirement
    {
        friend class iterator;

        pointer current = nullptr;

      private:
        static promise_type& from_prefix(handle_t coro) noexcept
        {
            if constexpr (is_clang)
            {
                // calculate the location of the coroutine frame prefix
                auto* prefix =
                    reinterpret_cast<clang_frame_prefix*>(coro.address());
                // for clang, promise is placed just after frame prefix
                auto* promise = reinterpret_cast<promise_type*>(prefix + 1);
                return *promise;
            }
            else if constexpr (is_msvc)
            {
                auto* prefix = reinterpret_cast<char*>(coro.address());
                auto* promise = reinterpret_cast<promise_type*>(
                    prefix - aligned_size_v<promise_type>);
                return *promise;
            }
            else
            {
                // !!! crash !!!
                return *reinterpret_cast<promise_type*>(nullptr);
            }
        }

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
        handle_t coro; // resumable handle

      public:
        // `enumerable::end()`
        iterator(std::nullptr_t) noexcept : coro{nullptr} {}
        // `enumerable::begin()`
        iterator(handle_t handle) noexcept : coro{handle} {}

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
            pointer ptr = promise_type::from_prefix(coro).current;
            return ptr;
        }
        pointer operator->() const noexcept
        {
            pointer ptr = promise_type::from_prefix(coro).current;
            return ptr;
        }
        reference operator*() noexcept { return *(this->operator->()); }
        reference operator*() const noexcept { return *(this->operator->()); }

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
