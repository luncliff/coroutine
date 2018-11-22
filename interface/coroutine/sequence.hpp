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

// #include <atomic>
#include <cassert>
#include <iterator>

template<typename T>
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
    handle_t coro{}; // resumable handle

  public:
    sequence(promise_type* ptr) noexcept : coro{nullptr}
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
    ~sequence() noexcept
    {
        // delete the coroutine frame
        if (coro) coro.destroy();
    }

  public:
    iterator begin() noexcept(false)
    {
        // The first and only moment for this function.
        // `resumeable_handle` must be guaranteed to be non-null
        // by `promise_type`. So we won't follow the scheme of `generator`

        // notice that promise_type used initial suspend
        coro.resume();

        // this line won't be true but will be remained for a while
        if (coro.done()) return nullptr;

        // check if it's finished after first resume
        auto& promise = promise_type::from_prefix(coro);
        if (promise.current == finished()) //
            return nullptr;

        return iterator{coro};
    }
    iterator end() noexcept { return nullptr; }

  public:
    struct promise_type
    {
        friend struct iterator;
        friend struct sequence;

        // static_assert(std::atomic<handle_t>::is_always_lock_free);

      public:
        pointer current = nullptr;
        handle_t task{}; // std::atomic<handle_t> task{};

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
        void unhandled_exception() noexcept { std::terminate(); }
        auto get_return_object() noexcept -> promise_type* { return this; }

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
            return task.address() != nullptr;
            // auto order = std::memory_order::memory_order_acquire;
            // return task.load(order) != nullptr;
        }
        void await_suspend(handle_t rh) noexcept
        {
            // iterator will reactivate this
            task = rh;
            // auto order = std::memory_order::memory_order_release;
            // task.store(order);
        }
        void await_resume() noexcept
        {
            handle_t _task = task;
            task = nullptr; // std::swap(_task, task);

            if (_task)          // Resume if and only if
                _task.resume(); // there is a waiting work

            // auto order = std::memory_order::memory_order_acq_rel;
            // if (handle_t coro =
            //        task.exchange(nullptr, // prevent recursive activation
            //                      order))
            //    coro.resume(); // this must be iterator
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
        iterator(handle_t rh) noexcept : promise{nullptr}
        {
            // After some trial,
            // `coroutine_handle` became a source of tedious code.
            // So we will use `promise_type` directly
            promise = std::addressof(promise_type::from_prefix(rh));
        }

      public:
        iterator& operator++(int) = delete; // post increment
        iterator& operator++() noexcept(false)
        {
            // reset and advance
            promise->current = empty();

            // iterator resumes promise if it is suspended
            handle_t _task = promise->task;
            promise->task = nullptr;

            if (_task) _task.resume();

            // if (handle_t coro = promise->task.exchange(
            //        nullptr, // prevent recursive activation
            //        std::memory_order::memory_order_acq_rel))
            //    coro.resume(); // this must be promise

            return await_resume();
        }

        bool await_ready() const noexcept
        {
            // case finished:
            //    Must advance because there is nothing to resume this
            //    iterator
            // case yield:
            //    Continue the loop
            return promise ? promise->current != empty() : true;
        }
        void await_suspend(handle_t coro) noexcept
        {
            // case empty:
            //   Promise suspended for some reason. Wait for it to yield
            //   Expect promise to resume this iterator appropriately
            promise->task = coro;

            // auto order = std::memory_order::memory_order_release;
            // promise->task.store(coro, order);
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

        pointer operator->() noexcept { return promise->current; }
        pointer operator->() const noexcept { return promise->current; }
        reference operator*() noexcept { return *(this->operator->()); }
        reference operator*() const noexcept { return *(this->operator->()); }

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
