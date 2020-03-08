/**
 * @file coroutine/channel.hpp
 * @author github.com/luncliff (luncliff@gmail.com)
 * @copyright CC BY 4.0
 * 
 * @brief C++ Coroutines based channel. It's a simplified form of the channel in The Go Language
 */
#pragma once
#ifndef LUNCLIFF_COROUTINE_CHANNEL_HPP
#define LUNCLIFF_COROUTINE_CHANNEL_HPP
#include <mutex>
#include <tuple>
#if __has_include(<coroutine>) // C++ 20
#include <coroutine>
namespace coro {
using namespace std;

#elif __has_include(<coroutine/frame.h>)
#include <coroutine/frame.h>
namespace coro {
using namespace std;
using namespace std::experimental;

#elif __has_include(<experimental/coroutine>) // C++ 17
#include <experimental/coroutine>
namespace coro {
using namespace std;
using namespace std::experimental;

#else
#error "expect header <experimental/coroutine> or <coroutine/frame.h>"
namespace coro {

#endif

/**
 * @defgroup channel 
 * @note
 * The implementation of channel heavily rely on `friend` relationship.
 * The design may make the template code ugly, but is necessary 
 * because of 2 behaviors.
 * 
 * - `channel` is a synchronizes 2 awaitable types, 
 *   `channel_reader` and `channel_writer`.
 * - Those reader/writer exchanges their information 
 *   before their `resume` of each other.
 * 
 * If user code can become mess because of such relationship, 
 * it is strongly recommended to hide `channel` internally and open their own interfaces.
 *  
 */

/**
 * @brief Lockable without lock operation
 * @note `channel` uses lockable whenever read/write is requested.
 * If its object is used without race condition, such lock operation can be skipped.
 * Use this **bypass** lockable for such cases.
 */
struct bypass_mutex final {
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
 * @note Notice that `reinterpret_cast` is not constexpr for some compiler.
 * @return void* non-null address
 * @ingroup channel
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

template <typename T, typename M = bypass_mutex>
class channel; // by default, channel doesn't care about the race condition
template <typename T, typename M>
class channel_reader;
template <typename T, typename M>
class channel_writer;
template <typename T, typename M>
class channel_peeker;

/**
 * @brief Awaitable type for `channel`'s read operation. 
 * It moves the value from writer coroutine's frame to reader coroutine's frame.
 * 
 * @code
 * void read_from(channel<int>& ch, int& ref, bool ok = false) {
 *     tie(ref, ok) = co_await ch.read();
 *     if(ok == false)
 *         ; // channel is under destruction !!!
 * }
 * @endcode
 * 
 * @tparam T type of the element
 * @tparam M mutex for the channel
 * @see channel_writer
 * @ingroup channel
 */
template <typename T, typename M>
class channel_reader {
  public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using channel_type = channel<T, M>;

  private:
    using reader_list = typename channel_type::reader_list;
    using writer = typename channel_type::writer;
    using writer_list = typename channel_type::writer_list;

    friend channel_type;
    friend writer;
    friend reader_list;

  protected:
    mutable pointer ptr; /// Address of value
    mutable void* frame; /// Resumeable Handle
    union {
        channel_reader* next = nullptr; /// Next reader in channel
        channel_type* chan;             /// Channel to push this reader
    };

  protected:
    explicit channel_reader(channel_type& ch) noexcept(false)
        : ptr{}, frame{nullptr}, chan{addressof(ch)} {
    }
    channel_reader(const channel_reader&) noexcept = delete;
    channel_reader& operator=(const channel_reader&) noexcept = delete;
    channel_reader(channel_reader&&) noexcept = delete;
    channel_reader& operator=(channel_reader&&) noexcept = delete;

  public:
    ~channel_reader() noexcept = default;

  public:
    /**
     * @brief Lock the channel and find available `channel_writer`
     * 
     * @return true   Matched with `channel_writer`
     * @return false  There was no available `channel_writer`. 
     *                The channel will be **lock**ed for this case.
     */
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
    /**
     * @brief Push to the channel and wait for `channel_writer`.
     * @note  The channel will be **unlock**ed after return. 
     * @param coro Remember current coroutine's handle to resume later
     * @see await_ready
     */
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        // notice that next & chan are sharing memory
        channel_type& ch = *(this->chan);
        // remember handle before push/unlock
        this->frame = coro.address();
        this->next = nullptr;
        // push to channel
        ch.reader_list::push(this);
        ch.mtx.unlock();
    }
    /**
     * @brief Returns value from writer coroutine, and `bool` indicator for the associtated channel's destruction
     * 
     * @return tuple<value_type, bool> 
     */
    auto await_resume() noexcept(false) -> tuple<value_type, bool> {
        auto t = make_tuple(value_type{}, false);
        // frame holds poision if the channel is under destruction
        if (this->frame == internal::poison())
            return t;
        // the resume operation can destroy the other coroutine
        // store before resume
        get<0>(t) = move(*ptr);
        if (auto coro = coroutine_handle<void>::from_address(frame))
            coro.resume();
        get<1>(t) = true;
        return t;
    }
};

/**
 * @brief Awaitable for `channel`'s write operation.
 * It exposes a reference to the value for `channel_reader`.
 * 
 * @code
 * void write_to(channel<int>& ch, int value) {
 *     bool ok = co_await ch.write(value);
 *     if(ok == false)
 *         ; // channel is under destruction !!!
 * }
 * @endcode
 * 
 * @tparam T type of the element
 * @tparam M mutex for the channel
 * @see channel_reader
 * @ingroup channel
 */
template <typename T, typename M>
class channel_writer {
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
    friend writer_list;
    friend peeker; // for `peek()` implementation

  private:
    mutable pointer ptr; /// Address of value
    mutable void* frame; /// Resumeable Handle
    union {
        channel_writer* next = nullptr; /// Next writer in channel
        channel_type* chan;             /// Channel to push this writer
    };

  private:
    explicit channel_writer(channel_type& ch, pointer pv) noexcept(false)
        : ptr{pv}, frame{nullptr}, chan{addressof(ch)} {
    }
    channel_writer(const channel_writer&) noexcept = delete;
    channel_writer& operator=(const channel_writer&) noexcept = delete;
    channel_writer(channel_writer&&) noexcept = delete;
    channel_writer& operator=(channel_writer&&) noexcept = delete;

  public:
    ~channel_writer() noexcept = default;

  public:
    /**
     * @brief Lock the channel and find available `channel_reader`
     * 
     * @return true   Matched with `channel_reader`
     * @return false  There was no available `channel_reader`. 
     *                The channel will be **lock**ed for this case.
     */
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
    /**
     * @brief Push to the channel and wait for `channel_reader`.
     * @note  The channel will be **unlock**ed after return. 
     * @param coro Remember current coroutine's handle to resume later
     * @see await_ready
     */
    void await_suspend(coroutine_handle<void> coro) noexcept(false) {
        // notice that next & chan are sharing memory
        channel_type& ch = *(this->chan);

        this->frame = coro.address(); // remember handle before push/unlock
        this->next = nullptr;         // clear to prevent confusing

        ch.writer_list::push(this); // push to channel
        ch.mtx.unlock();
    }
    /**
     * @brief Returns `bool` indicator for the associtated channel's destruction
     * 
     * @return true   successfully sent the value to `channel_reader`
     * @return false  The `channel` is under destruction
     */
    bool await_resume() noexcept(false) {
        // frame holds poision if the channel is under destruction
        if (this->frame == internal::poison())
            return false;
        if (auto coro = coroutine_handle<void>::from_address(frame))
            coro.resume();
        return true;
    }
};

/**
 * @brief C++ Coroutines based channel
 * @note  It works as synchronizer of `channel_writer`/`channel_reader`.
 *        The parameter mutex must meet the requirement of the synchronization. 
 * 
 * @tparam T type of the element
 * @tparam M Type of the mutex(lockable) for its member
 * @ingroup channel
 */
template <typename T, typename M>
class channel final : internal::list<channel_reader<T, M>>,
                      internal::list<channel_writer<T, M>> {
    static_assert(is_reference<T>::value == false,
                  "reference type can't be channel's value_type.");

  public:
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using mutex_type = M;

  private:
    using reader = channel_reader<value_type, mutex_type>;
    using reader_list = internal::list<reader>;
    using writer = channel_writer<value_type, mutex_type>;
    using writer_list = internal::list<writer>;
    using peeker = channel_peeker<value_type, mutex_type>;

    friend reader;
    friend writer;
    friend peeker; // for `peek()` implementation

  private:
    mutex_type mtx{};

  private:
    channel(const channel&) noexcept(false) = delete;
    channel(channel&&) noexcept(false) = delete;
    channel& operator=(const channel&) noexcept(false) = delete;
    channel& operator=(channel&&) noexcept(false) = delete;

  public:
    /**
     * @brief initialized 2 linked list and given mutex
     */
    channel() noexcept(false) : reader_list{}, writer_list{}, mtx{} {
    }

    /**
     * @brief Resume all attached coroutine read/write operations
     * @note Channel can't provide exception guarantee 
     * since the destruction contains coroutines' resume
     * 
     * If the channel is raced hardly, some coroutines can be
     * enqueued into list just after this destructor unlocks mutex.
     *
     * Unfortunately, this can't be detected at once since
     * we have 2 list (readers/writers) in the channel.
     *
     * Current implementation allows checking repeatedly to reduce the
     * probability of such interleaving.
     * **Modify the repeat count in the code** if the situation occurs.
     */
    ~channel() noexcept(false) {
        void* closing = internal::poison();
        writer_list& writers = *this;
        reader_list& readers = *this;
        // even 5'000+ can be unsafe for hazard usage ...
        size_t repeat = 1;
        do {
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
        } while (repeat--);
    }

  public:
    /**
     * @brief construct a new writer which references this channel
     * 
     * @param ref `T&` which holds a value to be `move`d to reader.
     * @return channel_writer
     */
    decltype(auto) write(reference ref) noexcept(false) {
        return channel_writer{*this, addressof(ref)};
    }
    /**
     * @brief construct a new reader which references this channel
     * 
     * @return channel_reader 
     */
    decltype(auto) read() noexcept(false) {
        return channel_reader{*this};
    }
};

/**
 * @brief Extension of `channel_reader` for subroutines
 * 
 * @tparam T type of the element
 * @tparam M mutex for the channel
 * @see channel_reader
 * @ingroup channel
 */
template <typename T, typename M>
class channel_peeker final : protected channel_reader<T, M> {
    using channel_type = channel<T, M>;
    using writer = typename channel_type::writer;

  private:
    channel_peeker(const channel_peeker&) noexcept(false) = delete;
    channel_peeker(channel_peeker&&) noexcept(false) = delete;
    channel_peeker& operator=(const channel_peeker&) noexcept(false) = delete;
    channel_peeker& operator=(channel_peeker&&) noexcept(false) = delete;

  public:
    explicit channel_peeker(channel_type& ch) noexcept(false)
        : channel_reader<T, M>{ch} {
    }
    ~channel_peeker() noexcept = default;

  public:
    /**
     * @brief Since there is no suspension for the `peeker`, 
     * the implementation will use scoped locking
     */
    void peek() const noexcept(false) {
        unique_lock lck{this->chan->mtx};
        if (this->chan->writer_list::is_empty() == false) {
            writer* w = this->chan->writer_list::pop();
            swap(this->ptr, w->ptr);
            swap(this->frame, w->frame);
        }
    }
    /**
     * @brief Move a value from matches `writer` to designated storage. After then, resume the `writer` coroutine.
     * @note  Unless the caller has invoked `peek`, nothing will happen.
     * 
     * @code
     * bool peek_channel(channel<string>& ch, string& out){
     *    peeker p{ch};
     *    p.peek();
     *    return p.acquire(out);
     * }
     * @endcode
     * 
     * @param storage memory object to store the value from `writer`
     * @return true   Acquired the value 
     * @return false  No `writer` found or `peek` is not invoked
     * @see peek
     */
    bool acquire(T& storage) noexcept(false) {
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
 * @note If the channel is readable, acquire the value and invoke the function
 * 
 * @see channel_peeker
 * @ingroup channel
 */
template <typename T, typename M, typename Fn>
void select(channel<T, M>& ch, Fn&& fn) noexcept(false) {
    static_assert(sizeof(channel_reader<T, M>) == sizeof(channel_peeker<T, M>));
    channel_peeker p{ch};   // peeker will move element
    T storage{};            //    into the call stack
    p.peek();               // the channel has waiting writer?
    if (p.acquire(storage)) // acquire + resume writer
        fn(storage);        // invoke the function
}

/**
 * @note For each pair, peeks a channel and invoke the function with the value if the peek was successful.
 * 
 * @ingroup channel
 * @see test/channel_select_type.cpp
 */
template <typename... Args, typename Ch, typename Fn>
void select(Ch& ch, Fn&& fn, Args&&... args) noexcept(false) {
    select(ch, forward<Fn&&>(fn));           // evaluate
    return select(forward<Args&&>(args)...); // try next pair
}

} // namespace coro

#endif // LUNCLIFF_COROUTINE_CHANNEL_HPP
