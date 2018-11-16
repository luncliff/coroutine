// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      The abstraction for the async generator
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_SEQUENCE_HPP
#define COROUTINE_SEQUENCE_HPP

#include <coroutine/frame.h>
#include <iterator>

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
        return reinterpret_cast<pointer>(0x0000);
    }

  private:
    handle_promise_t rh{}; // resumable handle

  public:
    sequence(promise_type* ptr) noexcept : rh{nullptr}
    {
        rh = handle_promise_t::from_promise(*ptr);
    }
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
    struct promise_type
    {
        friend struct iterator;

      public:
        pointer current = nullptr;
        handle_t task{};
        // ---- atomic implementation ----
        // static_assert(std::atomic<handle_t>::is_always_lock_free);
        // std::atomic<handle_t> task{};

      public:
        void unhandled_exception() noexcept { std::terminate(); }

        auto get_return_object() noexcept -> promise_type* { return this; }

        auto initial_suspend() /*const*/ noexcept
        {
            // Suspend immediately and let the iterator to resume
            return std::experimental::suspend_always{};
        }
        auto final_suspend() /*const*/ noexcept
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
            return task.address() != nullptr;
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
            handle_t coro{};
            std::swap(coro, task);
            if (coro)          // Resume if and only if
                coro.resume(); // there is a waiting work

            // ---- atomic implementation ----
            // if (handle_t rh =
            //        task.exchange(nullptr, // prevent recursive activation
            //                      std::memory_order::memory_order_acq_rel))
            //    rh.resume(); // this must be iterator
        }
    };

    struct iterator final
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = T;
        using reference = T const&;
        using pointer = T const*;

      public:
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
