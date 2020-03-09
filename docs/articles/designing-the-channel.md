
# Designing the coroutine channel

> commit: [`1.4.3`](https://github.com/luncliff/coroutine/tree/74467cb470a6bf8b9559a56ebdcb68ff915d871e)

This is a design note for the [`channel<T>`](../feature/channel-overview.md).

It's one of the oldest feature in this library. In this article, I will write about the design background of the type.

### Summary

[John Bandela](https://github.com/jbandela) already talked about [the coroutine based channel in CppCon 2016](https://www.youtube.com/watch?v=N3CkQu39j5I).
However, I designed [more limited one](../channel-overview.md) on my own.

```c++
#include <coroutine/channel.hpp>
using namespace coro;

constexpr int bye = 0;

auto consumer(channel<int>& ch) -> no_return {
    // the type doesn't support for-co_await for now
    for (auto [msg, ok] = co_await ch.read(); ok;
         tie(msg, ok) = co_await ch.read()) {
        // ok == true: we received a value
        if (msg == bye)
            break;

        co_await ch.write(msg);  // we can write in the reader coroutine
    }
}
auto producer_owner() -> no_return {
    channel<int> ch{};
    consumer(ch); // start a consumer routine

    for (int msg : {1, 2, 3, bye}) {
        auto ok = co_await ch.write(msg);
        // ok == true: we sent a value

        co_await ch.read(msg);  // we can read in the writer coroutine
    }
}
```

Notice that we are reading **before** writing values to the channel.
You can visit more example(test) codes for the type:

* [test/channel_read_write_nolock.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_read_write_nolock.cpp)
* [test/channel_read_fail_after_close.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_read_fail_after_close.cpp)
* [test/channel_write_read_nolock.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_write_read_nolock.cpp)
* [test/channel_write_fail_after_close.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_write_fail_after_close.cpp)
* [test/channel_ownership_consumer.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_ownership_consumer.cpp)
* [test/channel_ownership_producer.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_ownership_producer.cpp)
* [test/channel_race_no_leak.cpp](https://github.com/luncliff/coroutine/blob/1.4.3/test/channel_race_no_leak.cpp)

## Note

### Motivation(Background)

There are multiple ways to deliver value(normally an object) from a routine to another routine.  
`channel<T>` is designed to play a role as one of them.

#### Before the coroutine

Before the C++ Coroutines becames available, there were 2 ways.

* `return` : from callee subroutine to caller subroutine
* `future<T>` since C++11 : `return` with synchronization(shared state)

`return` forwards a `T` type object from callee subroutine to caller subroutine.

C++ 11 `future<T>`, do the same work but uses a shared state to provide synchronization. 
As we know, such a synchronization is for multi-threaded code. 
One of those methods' limitation is that they can deliver only 1 object (of `T` type), only 1 time (with the `return`). Which makes inconvenience sometimes...

#### After the coroutine

By adopting coroutine to our world, now we can suspend, and get a new way of delivery.

* `task<T>`: 1 time delivery, from a coroutine to another coroutine
* `generator<T>`: multi-time delivery, from the generator coroutine to activator coroutine

`task<T>` is the simplest way to get `co_return` from a coroutine. 
The most important design point for a coroutine function is that managing suspension(`co_await`).
It's because suspension is more general than finalization(`co_return`).
`task<T>` deals with the cases and activates(resumes) awaiting coroutine when the activatee coroutine returns.

`generator<T>` uses `co_yield` to deliver value from activatee(generator coroutine) to activator(resumer subroutine) **multiple** times.
For us, `co_yield` itself is a syntatic sugar that invokes `yield_value` and evaluate `co_await`(therefore, suspend) at the same time.
Indeed there is a harmony in the way `generator<T>` works. 

However, what if we can't acquire the value from `generator<T>` immediately?
What if advancing its iterator is asynchronous?

* `async_generator<T>`: `generator<T>` + awaitable iterator

So `async_generator<T>` allows us to attach a coroutine which will be resumed when the generator coroutine can yield(perform the delivery).
In my perspective, using `co_await` on its iterator is indeed a good idea for the interface design.

* Its user code remains very similar to that of the `generator<T>`
* The consumer coroutine is resumed automatically, so there is no coverage leak

You can review existing implementations.

> Please [let me know](https://github.com/luncliff/coroutine/issues) if there is another implementations so I can add them :)

* From the [https://github.com/kirkshoop/await](https://github.com/kirkshoop/await)
* In this library, which is based on kirkshoop's work, [`sequence<T>`](../yield-sequence.md)
* From the adorable [cppcoro](https://github.com/lewissbaker/cppcoro/blob/master/test/async_generator_tests.cpp#L67)

#### Another type of the delivery?

At this point, **we can recognize that we don't have a bidirectional delivery**.
`generator<T>` delivers its `co_yield`ed value only in 1 direction, and so does `async_generator<T>`.

Also, if we need to write multi-threaded code, there are still needs of the synchronization.
It depends on the pattern and can managed well, but it's still hard to the beginners.

For nondirectional delivery, there is a good example in the Go language. It's `channel`. 
The channel can suspend both producer and consumer goroutines by affecting their scheduling.

However, there is a difference between the coroutine in the C++ and Goroutine in the Go language.
Goroutine is scheduled by the Go runtime, but coroutine are not.
It doesn't have a built-in scheduling. 
Basically, `coroutine_handle<T>` itself is a pointer and programmers have to manage them manually.

### Requirement

So the requirement for our new type is like the following.

* `channel<T>`
    * Non-directional delivery (at least bidirectional)
    * Optional synchronization
    * Coverage leak prevention
    * Invalidation
    * Zero allocation

#### Non-directional delivery

We should be able to write/read to a channel in a same coroutine.  
It's convenient. And that is important for the beginners.

#### Optional synchronization

It supports single-threaded code and there must be zero-cost in the case.  

#### Coverage leak prevention

The type must prevent coverage leak.  
That means, it must do its best to make related coroutines reach their ends.


#### Invalidation

The channel must be able to notify its invalidation.  
So its user code can handle the operation failure and prevent undefined behavior while they are writing the code.

With the coverage requirement above, user will `co_return` the coroutine, or delegate its work somehow.

#### Zero allocation

`new`/`delete` is not allowed to avoid unnecessary cost.

### Design Concerns

#### Optional synchronization

The logic will remain while user applies different synchronization types.
So the type will use be template.

```c++
template <typename T, typename M = bypass_lock>
class channel; // by default, channel doesn't care about the race condition

template <typename T, typename M>
class channel final {
  public:
    using mutex_type = M; 
  private:
    mutex_type mtx;
}
```

Also, as you can see above, it will use **do nothing** lockable by default.

```c++
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
```

#### Invalidation

* unlike the channel of Go, the type doesn't provide explicit `close` operation

Explicit `close` is simple enough because we can just check the channel's state and break the loop.
But it is highly possible that user will carefully design the value if there is no `close`.

Suppose we have a `close` for the channel:
```c++
auto producer(channel<int>& ch) -> no_return {
    int msg{};
    // ...
    while(ch.closed() == false){
        co_await ch.write(msg);
        // ...
        if(cond)
            break;
    }
    ch.close(); // ok, no more value
    co_return;
}
auto consumer(channel<int>& ch) -> no_return {
    while(ch.closed() == false){
        int msg = co_await ch.read();
        // ...
    }
}
```

In this case, user will think about the state of the channel.  
The following questions can be managed by patternized code, but will always arise from the existance of `close`.

* Which line should the `close` placed?
* When it's already closed, what should I do for read/write?

When we don't have a `close`, users will think about another issue.

* When should I stop the read/write?

The question is about managing the loop, and probably they will use something like `EOF`(sentinel value).
In my perspective, it is more likely to have well-designed space for the `value_type` of the channel:

```c++
auto producer(channel<int>& ch) -> no_return {
    for (int msg : {1, 2, 3, bye}) {
        co_await ch.write(msg);
        // ...
    }
}

auto consumer(channel<int>& ch) -> no_return {
    int msg{};
    for(co_await ch.read(msg); msg != bye; 
        co_await ch.read(msg)){
        // ...
    }
}
```

### Implementation

Watching the [CppCon 2016 talk](https://www.youtube.com/watch?v=N3CkQu39j5I) will help you a lot for the following notes.

#### Making a linked list of the coroutine frames

Since coroutine frames are allocated separatly, we can use them like a node in the linked list.
By placing next pointer in those frame, we can make zero-allocation channel.
Actually they are a kind of pre-allocation since the cost is already paid in the frame construction(invocation) steps.

So it will be enough for `channel<T>` to have [2 linked lists and 1 mutex](https://github.com/luncliff/coroutine/blob/1.4.3/interface/coroutine/channel.hpp#L264) to operate correctly under multi-threaded code.

```c++
template <typename T, typename M>
class channel final : internal::list<reader<T, M>>,
                      internal::list<writer<T, M>> {
    // ...
  private:
    mutex_type mtx;
}
```

To make a linked list of the coroutine frames, 
the type **places an objects in the coroutine frame** that containes `next` pointer.

```c++
// Coroutine based channel. User have to provide appropriate lockable
template <typename T, typename M>
class channel final : internal::list<reader<T, M>>,
                      internal::list<writer<T, M>> {
    // ...
  public:
    // place a writer in the coroutine's frame
    decltype(auto) write(reference ref) noexcept(false) {
        return writer{*this, addressof(ref)};
    }
    // place a reader in the coroutine's frame
    decltype(auto) read() noexcept(false) {
        return reader{*this};
    }
};
```

You can find next pointer in the following `writer` and `reader`.
Notice that they have a reserved space to receive `coroutine_handle<void>` from `await_suspend`.

```c++
template <typename T, typename M>
class writer final {
  public:
    using value_type = T;
    using channel_type = channel<T, M>;

  private:
    mutable pointer ptr; // Address of value
    mutable void* frame;
    union {
        writer* next = nullptr; // Next writer in the channel
        channel_type* chan;
    };
};
```

```c++
template <typename T, typename M>
class reader {
  public:
    using value_type = T;
    using channel_type = channel<T, M>;

  protected:
    mutable pointer ptr; // Address of value
    mutable void* frame;
    union {
        reader* next = nullptr; // Next reader in the channel
        channel_type* chan;
    };
};
```

Because of the design we will place the objects in the frame. 

Consider:
```c++
auto temporary_reader_object(channel<int>& ch) -> no_return {
    // co_await ch.read();
    reader<int> reader = ch.read();
    co_await reader;
}
```

When it's used with `for` statement, there will be at most 2 read/writes.
So it won't be that complicated.

```c++
// at most 2 operation in 1 for loop

auto consumer(channel<int>& ch) -> no_return {
    for (auto [msg, ok] = co_await ch.read(); ok;
         tie(msg, ok) = co_await ch.read()) {
        // ...
    }
}

auto producer(channel<int>& ch) -> no_return {
    for (int msg : {1, 2, 3, bye}) {
        auto ok = co_await ch.write(msg);
        // ...
    }
}
```

#### Preventing Coverage Leak

We all know that RAII is the best way to make the type system work for us. Let's consider about it.

**What is the proper behavior of the linked list on its destruction?**  
Like the `std::list<T>`, it must delete its existing nodes to prevent memory leak.
Unfortunately, this case is **really special** because the nodes are existing coroutines' frame.

That means, if we free the node, it will destroy all objects in the frame.
That's really brutal because it can break all existing expectations of the function that manages its frame manually (with customized `promise_type`).

So the appropriate behavior is to resume those frames and let them recognize that they can't access the channel anymore.
In [its destructor](https://github.com/luncliff/coroutine/blob/1.4.3/interface/coroutine/channel.hpp#L296), `channel<T, M>` resumes all awaiting coroutines in its linked lists.
Notice the destructor is specified `noexcept(false)` because typically `resume` operation is not guaranteed `noexcept`.

```c++
template <typename T, typename M>
class channel {
    // ...
    ~channel() noexcept(false) // channel can't provide exception guarantee...
    {
        writer_list& writers = *this;
        reader_list& readers = *this;

        size_t repeat = 1;
        while (repeat--) {
            unique_lock lck{mtx};
            while (writers.is_empty() == false) {
                writer* w = writers.pop();
                auto coro = coroutine_handle<void>::from_address(w->frame);
                w->frame = internal::poison();  // <-- use a poison value

                coro.resume(); // <-- resume write operations
            }
            while (readers.is_empty() == false) {
                reader* r = readers.pop();
                auto coro = coroutine_handle<void>::from_address(r->frame);
                r->frame = internal::poison();

                coro.resume(); // <-- resume read operations
            }
        }
    }
}
```

With the behavior, user code must be changed like the following.
It receive not only a value from the channel but also one `bool` that notifies the channel is still accessible.

```c++
// Before
auto consumer(channel<int>& ch) -> no_return {
    int msg{};
    for(co_await ch.read(msg); msg != bye; 
        co_await ch.read(msg)){
        // ...
    }
}

// After
auto consumer(channel<int>& ch) -> no_return {
    // it's returning tuple, 
    //  declare memory objects using structured binding
    for (auto [msg, ok] = co_await ch.read(); ok;
         tie(msg, ok) = co_await ch.read()) {
        // ok == true: the channel is accessible
        // ...
    }
}
```

By doing so, if the reader/writer coroutines are written correctly, channel can guarantee there is no dangling coroutines in its linked lists, and prevent coverage leak of them.

#### Using a poison in `await_resume`

I used `poison` to replace explicit `close`.

Remember that the `co_await` expression is affected by `await_resume`.
Returning a `tuple` from the function can constrain user's code. 

In the implementation, to decide to return `true` or `false`, both `reader<T, M>` and `writer<T, M>` have to check if the poison is delivered.

```c++
// Awaitable for channel's read operation
template <typename T, typename M>
class reader {
    // ...
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
```

Unlike reader, writer just returns `bool`.
The signature makes its return can be used in `if` or `while` conveniently.

```c++
// Awaitable for channel's write operation
template <typename T, typename M>
class writer final {
    // ...
    bool await_resume() noexcept(false) {
        // frame holds poision if the channel is going to destroy
        if (this->frame == internal::poison())
            return false;

        if (auto coro = coroutine_handle<void>::from_address(frame))
            coro.resume();

        return true;
    }
};
```

Furthermore, the signature makes user to places a boolean in the coroutine's frame, **not in the channel**.
So it's on-demand cost.

#### Skip: `await_ready` and `await_suspend`

This part is realted to the requirement, optional synchronization.
It will be covered later because they are purely about the implementation.

If you are really curious, visit [this page](../channel-read_write.md).

### Conclusion

The type doesn't weighted for the performance.
What I focused was to enforce intuitive code and to guide users' consideration to their behavior (and undelying semantics).
It's not replacement nor alternative of the other delivery methods. 

To summarize each methods,

* `return` : 1 time, directional, subroutine to subroutine
* `future<T>` : 1 time, directional, subroutine to subroutine, synchronization(shared state)
* `task<T>`: 1 time, directional, coroutine to coroutine
* `generator<T>`: multi-time, directional, coroutine to coroutine
* `asnyc_generator<T>`: multi-time, directional, coroutine to coroutine, awaitable iterator
* `channel<T>`: multi-time (with suspend), non-directional, optional synchronization

For `channel<T>` itself...

#### Non-directional delivery

You can see the example with [the Compiler Explorer(MSVC)](https://godbolt.org/z/GXvfHK) or with [the WandBox(clang-8)](https://wandbox.org/permlink/6mRwvKtUAUzAKcdX)

#### Coverage Leak

For the requirement, channel becomes a linked list of each reader and writer.
Its operations place reader/writer object in the coroutine frame.

In the destructor, all objects in linked lists receive the poison value.

#### Invalidation (Close Status)

To detect invalidation of the channel, we check the poison value in the `await_resume` function.
It's signature enforces to make a memory object(`ok` in the example) to receive channel's status.

When the poison is delivered, `co_await` on the reader/writer will return `false`, so the user can break the loop.
This can be done without use of `close` operation.


