/**
 * @file coroutine/channel.hpp
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 * @brief Coroutine based channel. This is a simplified form of the channel in The Go Language
 */
#pragma once
#ifndef LUNCLIFF_COROUTINE_CHANNEL_HPP
#define LUNCLIFF_COROUTINE_CHANNEL_HPP

#if __has_include(<coroutine>) // C++ 20
#include <coroutine>
#elif __has_include(<coroutine/frame.h>)
#include <coroutine/frame.h>
#elif __has_include(<experimental/coroutine>) // C++ 17
#include <experimental/coroutine>
#else
#error "expect header <experimental/coroutine> or <coroutine/frame.h>"
#endif

#include <mutex>
#include <tuple>

namespace coro {
using namespace std;
using namespace std::experimental;

/**
 * @brief Lockable without lock operation
 * @details `channel` uses lockable whenever read/write is requested.
 * If its object is used without race condition, such lock operation can be skipped.
 * Use this **bypass** lockable for such cases.
 */
struct bypass_lock final {
    constexpr bool try_lock() noexcept {
        return true;
    }
    /** @brief Do nothing since this is 'bypass' lock */
    constexpr void lock() noexcept {
    }
    /** @brief Do nothing since it didn't locked something */
    constexpr void unlock() noexcept {
    }
};

namespace internal {

/**
 * @brief Returns a non-null address that leads access violation
 * @details Notice that `reinterpret_cast` is not constexpr for some compiler.
 * @return void* non-null address
 */
static void* poison() noexcept(false) {
    return reinterpret_cast<void*>(0xFADE'038C'BCFA'9E64);
}

/**
 * @brief Linked list without allocation
 * @tparam T Type of the node. Its member must have `next` pointer
 */
template <typename T>
class list {
    T* head{};
    T* tail{};

  public:
    bool is_empty() const noexcept(false) {
        return head == nullptr;
    }
    void push(T* node) noexcept(false) {
        if (tail) {
            tail->next = node;
            tail = node;
        } else
            head = tail = node;
    }
    /**
     * @return T* The return can be `nullptr`
     */
    auto pop() noexcept(false) -> T* {
        T* node = head;
        if (head == tail) // empty or 1
            head = tail = nullptr;
        else // 2 or more
            head = head->next;
        return node;
    }
};
} // namespace internal

template <typename T, typename M = bypass_lock>
class channel; // by default, channel doesn't care about the race condition
template <typename T, typename M>
class reader;
template <typename T, typename M>
class writer;
template <typename T, typename M>
class peeker;

/**
 * @brief Awaitable for `channel`'s read operation
 * 
 * @tparam T Element type
 * @tparam M mutex
 */
template <typename T, typename M>
class reader {
  public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using channel_type = channel<T, M>;

  private:
    using reader_list = typename channel_type::reader_list;
    using writer = typename channel_type::writer;
    using writer_list = typename channel_type::writer_list;
    using peeker = typename channel_type::peeker;

    friend channel_type;
    friend writer;
    friend peeker;
    friend reader_list;

  protected:
    mutable pointer ptr; /// Address of value
    mutable void* frame; /// Resumeable Handle
    union {
        reader* next = nullptr; /// Next reader in channel
        channel_type* chan;     /// Channel to push this reader
    };

  private:
    explicit reader(channel_type& ch) noexcept(false)
        : ptr{}, frame{nullptr}, chan{addressof(ch)} {
    }
    reader(const reader&) noexcept = delete;
    reader& operator=(const reader&) noexcept = delete;
    reader(reader&&) noexcept = delete;
    reader& operator=(reader&&) noexcept = delete;

  public:
    ~reader() noexcept = default;

  public:
    bool await_ready() const noexcept(false) {
        chan->mtx.lock();
        if (chan->writer_list::is_empty())
            // await_suspend will unlock in the case
            return false;

        writer* w = chan->writer_list::pop();
        // exchange address & resumeable_handle
        swap(this->ptr, w->ptr);
        swap(this->frame, w->frame);

        chan->mtx.unlock();
        return true;
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        // notice that next & chan are sharing memory
        channel_type& ch = *(this->chan);

        this->frame = coro.address(); // remember handle before push/unlock
        this->next = nullptr;         // clear to prevent confusing

        ch.reader_list::push(this); // push to channel
        ch.mtx.unlock();
    }
    auto await_resume() noexcept(false) -> tuple<value_type, bool> {
        auto t = make_tuple(value_type{}, false);
        // frame holds poision if the channel is going to be destroyed
        if (this->frame == internal::poison())
            return t;

        // Store first. we have to do this because the resume operation
        // can destroy the writer coroutine
        get<0>(t) = move(*ptr);
        if (auto coro = coroutine_handle<void>::from_address(frame))
            coro.resume();

        get<1>(t) = true;
        return t;
    }
};

/**
 * @brief Awaitable for `channel`'s write operation
 * 
 * @tparam T Element type
 * @tparam M mutex
 */
template <typename T, typename M>
class writer final {
  public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using channel_type = channel<T, M>;

  private:
    using reader = typename channel_type::reader;
    using reader_list = typename channel_type::reader_list;
    using writer_list = typename channel_type::writer_list;
    using peeker = typename channel_type::peeker;

    friend channel_type;
    friend reader;
    friend peeker;
    friend writer_list;

  private:
    mutable pointer ptr; /// Address of value
    mutable void* frame; /// Resumeable Handle
    union {
        writer* next = nullptr; /// Next writer in channel
        channel_type* chan;     /// Channel to push this writer
    };

  private:
    explicit writer(channel_type& ch, pointer pv) noexcept(false)
        : ptr{pv}, frame{nullptr}, chan{addressof(ch)} {
    }
    writer(const writer&) noexcept = delete;
    writer& operator=(const writer&) noexcept = delete;
    writer(writer&&) noexcept = delete;
    writer& operator=(writer&&) noexcept = delete;

  public:
    ~writer() noexcept = default;

  public:
    bool await_ready() const noexcept(false) {
        chan->mtx.lock();
        if (chan->reader_list::is_empty())
            // await_suspend will unlock in the case
            return false;

        reader* r = chan->reader_list::pop();
        // exchange address & resumeable_handle
        swap(this->ptr, r->ptr);
        swap(this->frame, r->frame);

        chan->mtx.unlock();
        return true;
    }
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        // notice that next & chan are sharing memory
        channel_type& ch = *(this->chan);

        this->frame = coro.address(); // remember handle before push/unlock
        this->next = nullptr;         // clear to prevent confusing

        ch.writer_list::push(this); // push to channel
        ch.mtx.unlock();
    }
    bool await_resume() noexcept(false) {
        // frame holds poision if the channel is going to destroy
        if (this->frame == internal::poison())
            return false;

        if (auto coro = coroutine_handle<void>::from_address(frame))
            coro.resume();

        return true;
    }
};

/**
 * @brief Coroutine based channel. User have to provide appropriate lockable
 * 
 * @tparam T Element type
 * @tparam M Type of the mutex(lockable) for its member
 */
template <typename T, typename M>
class channel final : internal::list<reader<T, M>>,
                      internal::list<writer<T, M>> {
    static_assert(is_reference<T>::value == false,
                  "reference type can't be channel's value_type.");

  public:
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using mutex_type = M;

  private:
    using reader = reader<value_type, mutex_type>;
    using reader_list = internal::list<reader>;
    using writer = writer<value_type, mutex_type>;
    using writer_list = internal::list<writer>;
    using peeker = peeker<value_type, mutex_type>;

    friend reader;
    friend writer;
    friend peeker;

  private:
    mutex_type mtx{};

  private:
    channel(const channel&) noexcept(false) = delete;
    channel(channel&&) noexcept(false) = delete;
    channel& operator=(const channel&) noexcept(false) = delete;
    channel& operator=(channel&&) noexcept(false) = delete;

  public:
    channel() noexcept(false) : reader_list{}, writer_list{}, mtx{} {
        // initialized 2 linked list and given mutex
    }

    /**
     * @brief Resume all attached coroutine read/write operations
     * @details Channel can't provide exception guarantee 
     * since the destruction contains coroutines' resume
     */
    ~channel() noexcept(false) {
        void* closing = internal::poison();
        writer_list& writers = *this;
        reader_list& readers = *this;
        /**
         * If the channel is raced hardly, some coroutines can be
         *  enqueued into list just after this destructor unlocks mutex.
         *
         * Unfortunately, this can't be detected at once since
         *  we have 2 list (readers/writers) in the channel.
         *
         * Current implementation allows checking repeatedly to reduce the
         * probability of such interleaving.
         * Increase the repeat count below if the situation occurs.
         * But notice that it is NOT zero.
         */
        size_t repeat = 1; // even 5'000+ can be unsafe for hazard usage ...
        while (repeat--) {
            unique_lock lck{mtx};
            while (writers.is_empty() == false) {
                writer* w = writers.pop();
                auto coro = coroutine_handle<void>::from_address(w->frame);
                w->frame = closing;

                coro.resume();
            }
            while (readers.is_empty() == false) {
                reader* r = readers.pop();
                auto coro = coroutine_handle<void>::from_address(r->frame);
                r->frame = closing;

                coro.resume();
            }
        }
    }

  public:
    decltype(auto) write(reference ref) noexcept(false) {
        return writer{*this, addressof(ref)};
    }
    decltype(auto) read() noexcept(false) {
        return reader{*this};
    }
};

/**
 * @brief Extension of `channel` reader for subroutines
 * 
 * @tparam T Element type
 * @tparam M mutex
 * @see reader
 */
template <typename T, typename M>
class peeker final : protected reader<T, M> {
    using value_type = T;
    using channel_type = channel<T, M>;
    using reader = typename channel_type::reader;
    using writer = typename channel_type::writer;

  private:
    peeker(const peeker&) noexcept(false) = delete;
    peeker(peeker&&) noexcept(false) = delete;
    peeker& operator=(const peeker&) noexcept(false) = delete;
    peeker& operator=(peeker&&) noexcept(false) = delete;

  public:
    explicit peeker(channel_type& ch) noexcept(false) : reader{ch} {
    }
    ~peeker() noexcept = default;

  public:
    void peek() const noexcept(false) {
        // since there is no suspension, use scoped locking
        unique_lock lck{this->chan->mtx};
        if (this->chan->writer_list::is_empty() == false) {
            writer* w = this->chan->writer_list::pop();
            swap(this->ptr, w->ptr);
            swap(this->frame, w->frame);
        }
    }
    bool acquire(value_type& storage) noexcept(false) {
        // if there was a writer, take its value
        if (this->ptr == nullptr)
            return false;
        storage = move(*this->ptr);

        // resume writer coroutine
        if (auto coro = coroutine_handle<void>::from_address(this->frame))
            coro.resume();
        return true;
    }
};

/**
 * @brief If the channel is readable, acquire the value and invoke the function
 * 
 * @tparam T 
 * @tparam M 
 * @tparam Fn 
 * @param ch 
 * @param fn 
 */
template <typename T, typename M, typename Fn>
void select(channel<T, M>& ch, Fn&& fn) noexcept(false) {
    static_assert(sizeof(reader<T, M>) == sizeof(peeker<T, M>));

    peeker<T, M> p{ch};     // peeker will move element
    T storage{};            //    into the call stack
    p.peek();               // the channel has waiting writer?
    if (p.acquire(storage)) // acquire + resume writer
        fn(storage);        // invoke the function
}

/**
 * @brief Invoke `select` for each pairs (channel + function)
 * 
 * @tparam Args 
 * @tparam ChanType 
 * @tparam FuncType 
 * @param ch 
 * @param fn 
 * @param args 
 */
template <typename... Args, typename ChanType, typename FuncType>
void select(ChanType& ch, FuncType&& fn, Args&&... args) noexcept(false) {
    select(ch, forward<FuncType&&>(fn));     // evaluate
    return select(forward<Args&&>(args)...); // try next pair
}

} // namespace coro

#endif // LUNCLIFF_COROUTINE_CHANNEL_HPP
