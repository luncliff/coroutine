// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      `enumerable` is an alias of `generator`.
//      Since the name is being used already, it uses another name
//
//      `sequence` is an abstraction for the async generator.
//      However, it is not named `async_generator` to imply that
//      it's just one of the concept's implementations
//
// ---------------------------------------------------------------------------
#ifndef COROUTINE_SEQUENCE_HPP
#define COROUTINE_SEQUENCE_HPP

#include <coroutine/frame.h>
#include <iterator>

namespace coro
{
using namespace std::experimental;

// - Note
//      Another implementation of <experimental/generator>
template <typename T>
class enumerable
{
  public:
    class promise_type;
    class iterator;

    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

  private:
    // alias resumable handle types
    using handle_promise_t = coroutine_handle<promise_type>;

    handle_promise_t coro;

  public:
    enumerable(promise_type* ptr) noexcept
        : coro{handle_promise_t::from_promise(*ptr)}
    {
    }
    enumerable(const enumerable&) = delete;
    enumerable& operator=(const enumerable&) = delete;
    enumerable(enumerable&& rhs) noexcept : coro{rhs.coro}
    {
        rhs.coro = nullptr;
    }
    enumerable& operator=(enumerable&& rhs)
    {
        std::swap(coro, rhs.coro);
        return *this;
    }
    ~enumerable() noexcept
    {
        // enumerable will destroy the frame.
        //  promise/iterator are free from those ownership
        if (coro)
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
            return suspend_always{};
        }
        auto final_suspend() const noexcept
        {
            return suspend_always{};
        }

        // `co_yield` expression. only for reference
        auto yield_value(reference ref) noexcept
        {
            current = std::addressof(ref);
            return suspend_always{};
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

template <typename T>
class sequence final
{
  public:
    class promise_type; // Resumable Promise Requirement
    class iterator;

    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

  private:
    using handle_promise_t = coroutine_handle<promise_type>;
    using handle_t = coroutine_handle<>;

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
    sequence(sequence&) = delete;
    sequence& operator=(sequence&) = delete;
    sequence(sequence&& rhs) noexcept : coro{rhs.coro}
    {
        rhs.coro = nullptr;
    }
    sequence& operator=(sequence&& rhs)
    {
        std::swap(coro, rhs.coro);
        return *this;
    }
    sequence(promise_type* ptr) noexcept
        : coro{handle_promise_t::from_promise(*ptr)}
    {
    }
    ~sequence() noexcept
    {
        // delete the coroutine frame
        //  coro.done() won't be checked since the usecase might want it
        //  the library only guarantees there is no leak
        if (coro)
            coro.destroy();
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
        if (coro.done())
            return iterator{nullptr};

        // check if it's finished after first resume
        auto& promise
            = handle_promise_t::from_address(coro.address()).promise();
        if (promise.current == finished()) //
            return iterator{nullptr};

        return iterator{coro};
    }
    iterator end() noexcept
    {
        return iterator{nullptr};
    }

  public:
    class promise_type final
    {
        friend class iterator;
        friend class sequence;

      public:
        pointer current{};
        handle_t task{};

      public:
        void unhandled_exception() noexcept
        {
            std::terminate();
        }
        auto get_return_object() noexcept -> promise_type*
        {
            return this;
        }

        auto initial_suspend() const noexcept
        {
            // Suspend immediately and let the iterator to resume
            return suspend_always{};
        }
        auto final_suspend() const noexcept
        {
            // Delete of `iterator` & `promise`
            //
            // Unfortunately, it's differ upon scenario that which one should
            //   deleted before the other.
            // Just suspend here and let them cooperate for it.
            //
            return suspend_always{};
        }

        promise_type& yield_value(reference ref) noexcept
        {
            current = std::addressof(ref);
            // case yield:
            //   iterator will take the value
            return *this;
        }
        template <typename Awaitable>
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
        }
        void await_suspend(handle_t rh) noexcept
        {
            // iterator will reactivate this
            task = rh;
        }
        void await_resume() noexcept
        {
            handle_t _task = task;
            task = nullptr; // std::swap(_task, task);

            if (_task)          // Resume if and only if
                _task.resume(); // there is a waiting work
        }
    };

    class iterator final
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
        explicit iterator(std::nullptr_t) noexcept : promise{nullptr}
        {
        }
        explicit iterator(handle_t rh) noexcept : promise{nullptr}
        {
            auto& p = handle_promise_t::from_address(rh.address()).promise();

            // After some trial,
            // `coroutine_handle` became a source of tedious code.
            // So we will use `promise_type` directly
            promise = std::addressof(p);
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

            if (_task)
                _task.resume();

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
        void await_suspend(handle_t rh) noexcept
        {
            // case empty:
            //   Promise suspended for some reason. Wait for it to yield
            //   Expect promise to resume this iterator appropriately
            promise->task = rh;
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

        pointer operator->() noexcept
        {
            return promise->current;
        }
        pointer operator->() const noexcept
        {
            return promise->current;
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
            return this->promise == rhs.promise;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };
};
} // namespace coro

#endif // COROUTINE_SEQUENCE_HPP
