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

//#include <atomic>

template<typename T, typename A = std::allocator<uint64_t>>
struct sequence final
{
    struct promise_type; // Resumable Promise Requirement
    struct iterator;

    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

  private:
    using handle_promise_t = std::experimental::coroutine_handle<promise_type>;
    using handle_t = std::experimental::coroutine_handle<>;

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
        // delete the coroutine frame
        if (rh) rh.destroy();
    }

  public:
    iterator begin() noexcept(false)
    {
        // The first and only moment for this function.
        // `resumeable_handle` must be guaranteed to be non-null
        // by `promise_type`. So we won't follow the scheme of `generator`

        // notice that promise_type used initial suspend
        rh.resume();

        // this line won't be true but will be remained for a while
        if (rh.done()) return nullptr;

        // check if it's finished after first resume
        if (rh.promise().current == finished()) //
            return nullptr;

        return iterator{rh};
    }
    iterator end() noexcept { return nullptr; }

  public:
    struct promise_type final
    {
        friend struct iterator;

      public:
        pointer current = nullptr;
        handle_t task{};
        // ---- atomic implementation ----
        // static_assert(std::atomic<handle_t>::is_always_lock_free);
        // std::atomic<handle_t> task{};

      private:
        promise_type(promise_type&) = delete;
        promise_type(promise_type&&) = delete;
        promise_type& operator=(promise_type&) = delete;
        promise_type& operator=(promise_type&&) = delete;

      public:
        promise_type() noexcept = default;
        ~promise_type() noexcept = default;

      public:
        void unhandled_exception() noexcept { std::terminate(); }
        auto get_return_object() noexcept -> handle_promise_t
        {
            return handle_promise_t::from_promise(*this);
        }

        auto initial_suspend() const noexcept
        {
            // Suspend immediately and let the iterator to resume
            return std::experimental::suspend_always{};
        }
        auto final_suspend() const noexcept
        {
            // Delete of `iterator` & `promise`
            //
            // Unfortunately, it's differ upon scenario that which one should
            //   deleted before the other.
            // Just suspend here and let them cooperate for it.
            //
            return std::experimental::suspend_always{};
        }

        promise_type& yield_value(reference ref) noexcept
        {
            current = std::addressof(ref);
            // case yield:
            //   iterator will take the value
            return *this;
        }
        template<typename Awaitable>
        Awaitable& yield_value(Awaitable&& a) noexcept
        {
            current = empty();
            // case empty:
            //    The given awaitable will reactivate this.
            //    NOTICE that iterator is going to suspend after this
            //    AND there is a short gap before the suspend of the iterator
            //
            //    The gap can be critical for multi-threaded scenario
            //
            return a;
        }
        void return_void() noexcept
        {
            current = finished();
            // case finished:
            //    Activate the iterator anyway.
            //    It's not a good pattern but this point is the appropriate for
            //    the iterator to notice and stop the loop
            this->await_resume();
        }

        bool await_ready() const noexcept
        {
            // Never suspend if there is a waiting iterator.
            // It will be resumed in `await_resume` function.
            //
            // !!! Using relaxed order here needs more verification !!!
            //
            return task != nullptr;
            // ---- atomic implementation ----
            // return task.load(std::memory_order::memory_order_acquire) !=
            //       nullptr;
        }
        void await_suspend(handle_t rh) noexcept
        {
            // iterator will reactivate this
            task = rh;
            // task.store(rh, std::memory_order::memory_order_release);
        }
        void await_resume() noexcept
        {
            // Resume if and only if there is a waiting work
            handle_t rh{};
            std::swap(rh, task);
            if (rh) rh.resume();
            // ---- atomic implementation ----
            // if (handle_t rh =
            //        task.exchange(nullptr, // prevent recursive activation
            //                      std::memory_order::memory_order_acq_rel))
            //    rh.resume(); // this must be iterator
        }
    };

    struct iterator final
    {
        promise_type* promise{};

      public:
        iterator(std::nullptr_t) noexcept : promise{nullptr} {}
        iterator(handle_promise_t rh) noexcept
            : promise{std::addressof(rh.promise())}
        {
            // After some trial,
            // `coroutine_handle` became a source of tedious code.
            // So we will use `promise_type` directly
        }

      public:
        iterator& operator++() noexcept(false)
        {
            // reset and advance
            promise->current = empty();

            // iterator resumes promise if it is suspended
            handle_t rh{};
            std::swap(rh, promise->task);
            if (rh) rh.resume();
            // ---- atomic implementation ----
            // if (handle_t rh = promise->task.exchange(
            //        nullptr, // prevent recursive activation
            //        std::memory_order::memory_order_acq_rel))
            //    rh.resume(); // this must be promise

            // see the function...
            return await_resume();
        }
        // disable post increment
        iterator& operator++(int) = delete;

        bool await_ready() const noexcept
        {
            // case finished:
            //    Must advance because there is nothing to resume this
            //    iterator
            // case yield:
            //    Continue the loop
            if (promise)
                return promise->current != empty();
            else
                return true;
        }
        void await_suspend(handle_t rh) noexcept
        {
            // case empty:
            //   Promise suspended for some reason. Wait for it to yield
            //   Expect promise to resume this iterator appropriately
            promise->task = rh;
            // ---- atomic implementation ----
            // promise->task.store(rh, std::memory_order::memory_order_release);
        }
        iterator& await_resume() noexcept
        {
            // case finished:
            //    There is nothing we can do in this iterator.
            //    We have meet the end condition of `for-co_await`
            if (promise &&                      //
                promise->current == finished()) //
                promise = nullptr;              // forget the promise

            return *this;
        }

        bool operator==(const iterator& rhs) const noexcept
        {
            // if iterator is finished, it's promise must be nullptr
            return this->promise == rhs.promise;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        // Expect copy semantics for now.
        // These will be updated after some test...

        auto operator-> () noexcept { return promise->current; }
        auto operator-> () const noexcept { return promise->current; }
        decltype(auto) operator*() noexcept { return *(promise->current); }
        decltype(auto) operator*() const noexcept
        {
            return *(promise->current);
        }
    };
};

#endif // COROUTINE_SEQUENCE_HPP
