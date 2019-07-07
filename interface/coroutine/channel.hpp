// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Note
//      Coroutine based channel
//      This is a simplified form of channel in The Go Language
//
// ---------------------------------------------------------------------------
#pragma once
#ifndef LUNCLIFF_COROUTINE_CHANNEL_HPP
#define LUNCLIFF_COROUTINE_CHANNEL_HPP

#include <mutex>
#include <tuple>

#include <coroutine/frame.h>

namespace coro {
using namespace std;
using namespace std::experimental;

// Lockable without lock operation.
struct bypass_lock final {
    constexpr bool try_lock() noexcept {
        return true;
    }
    constexpr void lock() noexcept {
        // do nothing since this is 'bypass' lock
    }
    constexpr void unlock() noexcept {
        // it is not locked
    }
};

namespace internal {

// A non-null address that leads access violation
static inline void* poison() noexcept(false) {
    return reinterpret_cast<void*>(0xFADE'038C'BCFA'9E64);
}

// Linked list without allocation
template <typename T>
class list {
    using node_type = T;

    node_type* head{};
    node_type* tail{};

  public:
    list() noexcept = default;

  public:
    bool is_empty() const noexcept(false) {
        return head == nullptr;
    }
    void push(node_type* node) noexcept(false) {
        if (tail) {
            tail->next = node;
            tail = node;
        } else
            head = tail = node;
    }
    auto pop() noexcept(false) -> node_type* {
        node_type* node = head;
        if (head == tail) // empty or 1
            head = tail = nullptr;
        else // 2 or more
            head = head->next;

        return node; // this can be nullptr
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

// Awaitable for channel's read operation
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
    mutable pointer ptr; // Address of value
    mutable void* frame; // Resumeable Handle
    union {
        reader* next = nullptr; // Next reader in channel
        channel_type* chan;     // Channel to push this reader
    };

  private:
    explicit reader(channel_type& ch) noexcept(false)
        : ptr{}, frame{nullptr}, chan{addressof(ch)} {
    }
    reader(const reader&) noexcept(false) = delete;
    reader& operator=(const reader&) noexcept(false) = delete;

  public:
    reader(reader&& rhs) noexcept(false) {
        swap(this->ptr, rhs.ptr);
        swap(this->frame, rhs.frame);
        swap(this->chan, rhs.chan);
    }
    reader& operator=(reader&& rhs) noexcept(false) {
        swap(this->ptr, rhs.ptr);
        swap(this->frame, rhs.frame);
        swap(this->chan, rhs.chan);
        return *this;
    }
    ~reader() noexcept = default;

  public:
    bool await_ready() const noexcept(false) {
        chan->mtx.lock();
        if (chan->writer_list::is_empty())
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
        auto& value = get<0>(t);
        value = move(*ptr);
        if (auto coro = coroutine_handle<void>::from_address(frame))
            coro.resume();

        get<1>(t) = true;
        return t;
    }
};

// Awaitable for channel's write operation
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
    mutable pointer ptr; // Address of value
    mutable void* frame; // Resumeable Handle
    union {
        writer* next = nullptr; // Next writer in channel
        channel_type* chan;     // Channel to push this writer
    };

  private:
    explicit writer(channel_type& ch, pointer pv) noexcept(false)
        : ptr{pv}, frame{nullptr}, chan{addressof(ch)} {
    }
    writer(const writer&) noexcept(false) = delete;
    writer& operator=(const writer&) noexcept(false) = delete;

  public:
    writer(writer&& rhs) noexcept(false) {
        swap(this->ptr, rhs.ptr);
        swap(this->frame, rhs.frame);
        swap(this->chan, rhs.chan);
    }
    writer& operator=(writer&& rhs) noexcept(false) {
        swap(this->ptr, rhs.ptr);
        swap(this->frame, rhs.frame);
        swap(this->chan, rhs.chan);
        return *this;
    }
    ~writer() noexcept = default;

  public:
    bool await_ready() const noexcept(false) {
        chan->mtx.lock();
        if (chan->reader_list::is_empty())
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

// Coroutine based channel. User have to provide appropriate lockable
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

  public:
    channel(const channel&) noexcept(false) = delete;
    channel(channel&&) noexcept(false) = delete;
    channel& operator=(const channel&) noexcept(false) = delete;
    channel& operator=(channel&&) noexcept(false) = delete;
    channel() noexcept(false) : reader_list{}, writer_list{}, mtx{} {
    }
    ~channel() noexcept(false) // channel can't provide exception guarantee...
    {
        writer_list& writers = *this;
        reader_list& readers = *this;
        //
        // If the channel is raced hardly, some coroutines can be
        //  enqueued into list just after this destructor unlocks mutex.
        //
        // Unfortunately, this can't be detected at once since
        //  we have 2 list (readers/writers) in the channel.
        //
        // Current implementation allows checking repeatedly to reduce the
        //  probability of such interleaving.
        // Increase the repeat count below if the situation occurs.
        // But notice that it is NOT zero.
        //
        size_t repeat = 1; // author experienced 5'000+ for hazard usage
        while (repeat--) {
            unique_lock lck{mtx};

            while (writers.is_empty() == false) {
                writer* w = writers.pop();
                auto coro = coroutine_handle<void>::from_address(w->frame);
                w->frame = internal::poison();

                coro.resume();
            }
            while (readers.is_empty() == false) {
                reader* r = readers.pop();
                auto coro = coroutine_handle<void>::from_address(r->frame);
                r->frame = internal::poison();

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

// Extension of channel reader for subroutines
template <typename T, typename M>
class peeker final : protected reader<T, M> {
    using value_type = T;
    using channel_type = channel<T, M>;
    using reader = typename channel_type::reader;
    using writer = typename channel_type::writer;

  public:
    explicit peeker(channel_type& ch) noexcept(false) : reader{ch} {
    }
    peeker(const peeker&) noexcept(false) = delete;
    peeker(peeker&& rhs) noexcept(false) = delete;
    peeker& operator=(const peeker&) noexcept(false) = delete;
    peeker& operator=(peeker&& rhs) noexcept(false) = delete;
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

//  If the channel is readable, acquire the value and the function.
template <typename T, typename M, typename Fn>
void select(channel<T, M>& ch, Fn&& fn) noexcept(false) {
    static_assert(sizeof(reader<T, M>) == sizeof(peeker<T, M>));

    peeker<T, M> p{ch};     // peeker will move element
    T storage{};            //    into the call stack
    p.peek();               // the channel has waiting writer?
    if (p.acquire(storage)) // acquire + resume writer
        fn(storage);        // invoke the function
}

//  Invoke `select` for each pairs (channel + function)
template <typename... Args, typename ChanType, typename FuncType>
void select(ChanType& ch, FuncType&& fn, Args&&... args) noexcept(false) {
    select(ch, forward<FuncType&&>(fn));     // evaluate
    return select(forward<Args&&>(args)...); // try next pair
}

} // namespace coro

#endif // LUNCLIFF_COROUTINE_CHANNEL_HPP
