// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//  License
//      CC BY 4.0
//
//  Note
//      The abstraction for the async generator
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_SEQUENCE_HPP
#define COROUTINE_SEQUENCE_HPP

#include <experimental/coroutine>

#ifdef _MSC_VER

// #include <experimental/resumable>
#include <experimental/generator>

#elif __clang__ // clang compiler will use the followings...

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

    struct iterator
    //  : public std::iterator<std::input_iterator_tag, T>
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
        auto operator*() const noexcept -> const T &
        {
            return *(rh.promise().current);
        }

        auto operator-> () noexcept -> T * { return rh.promise().current; }
        auto operator-> () const noexcept -> const T *
        {
            return rh.promise().current;
        }

        bool operator==(const iterator &rhs) const noexcept
        {
            return this->rh == rhs.rh;
        }
        bool operator!=(const iterator &rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };
};

} // namespace experimental
} // namespace std

#endif // _WIN32 <experimental/generator>
#endif // COROUTINE_SEQUENCE_HPP
