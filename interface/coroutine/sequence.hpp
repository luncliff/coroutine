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

#ifdef __clang__

namespace std
{
namespace experimental
{
// - Note
//    This implementation should work like <experimental/generator> of VC++
template<typename T,
         typename A = allocator<char>> // byte level allocation by default
struct generator
{
    struct promise_type; // Resumable Promise Requirement
    struct iterator;     // Abstraction: iterator

  private:
    coroutine_handle<promise_type> rh{}; // resumable handle

  private:
    // copy
    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;

  public:
    generator(coroutine_handle<promise_type>&& handle_from_promise) noexcept
        : rh{std::move(handle_from_promise)}
    {
        // must ensure handle is valid...
    }

    // move
    generator(generator&& rhs) noexcept : rh{std::move(rhs.rh)} {}
    generator& operator=(generator&& rhs) noexcept
    {
        std::swap(this->rh, rhs.rh);
        return *this;
    }

    ~generator() noexcept
    {
        // generator will destroy the frame.
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
    // Resumable Promise Requirement
    struct promise_type
    {
      public:
        // iterator will access to this pointer
        //  and reference memory object in coroutine frame
        T* current = nullptr;

      private:
        // promise type is strongly coupled to coroutine frame.
        // so copy/move might create wrong(garbage) coroutine handle
        promise_type(promise_type&) noexcept = delete;
        promise_type(promise_type&&) = delete;

      public:
        promise_type() = default;
        ~promise_type() = default;

      public:
        // suspend at init/final suspension point
        auto initial_suspend() const noexcept { return suspend_always{}; }
        auto final_suspend() const noexcept { return suspend_always{}; }
        // `co_yield` expression. only for reference
        auto yield_value(T& ref) noexcept
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
    {
        coroutine_handle<promise_type> rh;

      public:
        // `generator::end()`
        iterator(nullptr_t) noexcept : rh{nullptr} {}
        // `generator::begin()`
        iterator(coroutine_handle<promise_type> handle) noexcept : rh{handle} {}

      public:
        iterator& operator++() noexcept(false)
        {
            rh.resume();
            // generator will destroy coroutine frame later...
            if (rh.done()) rh = nullptr;
            return *this;
        }

        // post increment
        iterator& operator++(int) = delete;

        auto operator*() noexcept -> T& { return *(rh.promise().current); }
        auto operator*() const noexcept -> const T&
        {
            return *(rh.promise().current);
        }

        auto operator-> () noexcept -> T* { return rh.promise().current; }
        auto operator-> () const noexcept -> const T*
        {
            return rh.promise().current;
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

} // namespace experimental
} // namespace std

#elif _MSC_VER // clang compiler will use the followings...

// #include <experimental/resumable>
#include <experimental/generator>

#endif // _WIN32 <experimental/generator>

#include <atomic>

template<typename T>
//, typename A = std::allocator<char>> // byte level allocation by default
struct sequence final
{
    struct promise_type; // Resumable Promise Requirement
    struct iterator;     // Abstraction: iterator

    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

    using handle_promise_t = std::experimental::coroutine_handle<promise_type>;
    using handle_t = std::experimental::coroutine_handle<>;

    static_assert(std::atomic<handle_t>::is_always_lock_free);

  private:
    static constexpr pointer finished() noexcept
    {
        return reinterpret_cast<pointer>(0xDEAD);
    }
    static constexpr pointer empty() noexcept
    {
        return reinterpret_cast<pointer>(nullptr);
    }

  private:
    handle_promise_t rh{}; // resumable handle

  public:
    sequence(handle_promise_t&& handle) noexcept : rh{std::move(handle)} {}
    ~sequence() noexcept
    {
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

            if (rh.promise().current == finished()) //
                return nullptr;
        }
        return iterator{rh};
    }
    iterator end() noexcept { return iterator{nullptr}; }

  public:
    struct promise_type final
    {
        friend struct iterator;

      public:
        std::atomic<handle_t> task{};
        // handle_t task;
        pointer current = nullptr;

      private:
        promise_type(promise_type&) = delete;
        promise_type(promise_type&&) = delete;
        promise_type& operator=(promise_type&) = delete;
        promise_type& operator=(promise_type&&) = delete;

      public:
        promise_type() = default;
        ~promise_type() noexcept = default;

      public:
        void unhandled_exception() noexcept { std::terminate(); }
        auto get_return_object() noexcept -> handle_promise_t
        {
            return handle_promise_t::from_promise(*this);
        }

        auto initial_suspend() const noexcept
        {
            return std::experimental::suspend_always{};
        }
        auto final_suspend() const noexcept
        {
            return std::experimental::suspend_always{};
        }

        promise_type& yield_value(reference ref) noexcept
        {
            current = std::addressof(ref);
            return *this;
        }

        template<typename Awaitable>
        Awaitable& yield_value(Awaitable&& a) noexcept
        {
            current = empty();
            return a; // the given awaitable will reactivate this coroutine
        }

        void return_void() noexcept
        {
            current = finished();
            // final suspend point seems not appropriate
            // for finishing the iterator.
            this->await_resume();
        }

        bool await_ready() const noexcept
        {
            // using relaxed order here needs more verification...
            return task.load(std::memory_order::memory_order_acquire) !=
                   nullptr;
        }
        void await_suspend(handle_t handle) noexcept
        {
            task.store(handle, std::memory_order::memory_order_release);
        }
        void await_resume() noexcept
        {
            // we can use `atomic_exchange`,
            //   but will separate that to make it more explicit

            handle_t work = task.load(std::memory_order::memory_order_acquire);
            task.store(nullptr, std::memory_order::memory_order_release);

            if (work)          // resume if and only if there is a waiting work
                work.resume(); // it should be iterator
        }
    };

    struct iterator final
    {
        promise_type* promise{};

      public:
        iterator(std::nullptr_t) noexcept : promise{nullptr} {}
        iterator(handle_promise_t handle) noexcept
            : promise{std::addressof(handle.promise())}
        {
        }

      public:
        iterator& operator++() noexcept(false)
        {
            // reset and advance
            promise->current = empty();

            if (auto task = promise->task.load()) // promise suspended
                task.resume();

            // forget the promise so we can end the loop
            if (promise->current == finished()) promise = nullptr;

            return *this;
        }
        // post increment
        iterator& operator++(int) = delete;

        bool await_ready() const noexcept
        {
            // finished or yield
            return promise->current != empty();
        }
        void await_suspend(handle_t handle) noexcept
        {
            // promise will continue this work...
            // promise->task = handle;
            promise->task.store(handle,
                                std::memory_order::memory_order_release);
        }
        iterator& await_resume() noexcept
        {
            // If producer routine is finished,
            //   there is nothing we can do in this iterator
            //   forget the promise and so we can end the loop
            if (promise->current == finished()) //
                promise = nullptr;

            return *this;
        }

        auto operator*() noexcept { return *(promise->current); }
        auto operator*() const noexcept { return *(promise->current); }
        auto operator-> () noexcept { return promise->current; }
        auto operator-> () const noexcept { return promise->current; }
        bool operator==(const iterator& rhs) const noexcept
        {
            return this->promise == rhs.promise;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };
};

#endif // COROUTINE_SEQUENCE_HPP
