
# Awaitable event using the coroutine, `epoll`, and `eventfd`

> commit: [`ad1e682f`](https://github.com/luncliff/coroutine/tree/ad1e682fe9a01a250e71080366d653af60eda708)

The note explains the detail of [`event` in `coroutine/concrt.h`](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/interface/coroutine/concrt.h#L194)

### Summary

Look & feel of the [interface](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/interface/coroutine/concrt.h#L194) via [test code](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/test/c2_concrt_event.cpp).

```c++
auto wait_for_one_event(event& e, atomic_flag& flag) -> no_return {
  try {

    // resume after the event is signaled ...
    co_await e;

  } catch (system_error& e) {
    // event throws if there was an internal system error
    FAIL(e.what());
  }
  flag.test_and_set();
}

TEST_CASE("wait for one event", "[event]") {
  event e1{};
  atomic_flag flag = ATOMIC_FLAG_INIT;

  wait_for_one_event(e1, flag);
  e1.set();

  auto count = 0;
  for (auto task : signaled_event_tasks()) {
    task.resume();
    ++count;
  }
  REQUIRE(count > 0);
  // already set by the coroutine `wait_for_one_event`
  REQUIRE(flag.test_and_set() == true);
}
```

## Note

### Motivation

It would be convenient if there is a simple event type for `co_await` operator.
Linux system's [`eventfd`](http://man7.org/linux/man-pages/man2/eventfd.2.html) might be able to do the work.

### Requirement

The requirement for the `event` type is simple. 

* It doesn’t support **copy** construction/assignment
* It doesn’t support **move** construction/assignment
* The type can’t be inherited(`final`)
* It can be an operand of `co_await` operator 
* The event is stateful and has 2 states. 
    * Signaled
    * Non-signaled
* For each state, the behavior is like the following
    * Non-signaled:  
      `co_await` will suspend the coroutine 
      and becomes resumable when the event object is signaled.
      If the event is already signaled, the coroutine must not suspend.
    * Signaled:  
      `co_await` won’t suspend
      and the event object becomes non-signaled state after the statement.

The first 3 requirement is quite strict, but it’s for simplicity. Normally it won’t be that hazardous for move operation. 
But moving from coroutine’s frame to another space is a tricky situation. So I’ve banned move semantics to prevent misusage like that. 

### Design

Win32 API supports [internal thread pool](https://docs.microsoft.com/en-us/windows/desktop/procthread/thread-pool-api), but Linux system API does not. 
So user code has to poll those created events. Fortunately, Linux supports [`epoll`](http://man7.org/linux/man-pages/man7/epoll.7.html) to allow the behavior.
We can derive 2 behavior constraint from the interface limitation.

* To acquire a list of signaled events, user code has to perform a polling operation
    * Limitation from `epoll`’s use-case
* User has to resume coroutines that are suspended for an event object
    * Limitation from `coroutine_handle<void>` and absence of embedded thread pool/APC support

This was the rough version of the interface type & function.

```c++
// Awaitable event type.
class event final : no_copy_move {
  public:
    using task = coroutine_handle<void>;

  private:
    uint64_t state; // it's lightweight !

  public:
    event();
    ~event();

    void set();

    bool await_ready() const;
    void await_suspend(coroutine_handle<void> coro);
    void await_resume();
};

//  Enumerate all suspended coroutines that are waiting for signaled events.
auto signaled_event_tasks()   -> coro::enumerable<event::task>;
```

> [`coro::enumerable<T>`](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/interface/coroutine/yield.hpp#L26) is my own implementation of the `generator<T>` in [`<experimental/generator>`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/n4736.pdf)

`state` is a space for `eventfd`, and `signaled_event_tasks` performs polling operation on the `epoll` file descriptor.

#### Concerns

You may think about why I didn’t adopt design like `io_context` in [Boost ASIO](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio.html). Which provides an explicit point of creation and polling operation.
For instance, with Boost ASIO, user code must create objects(e.g, socket) via `boost::asio::io_context` object.

```c++
// see: https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/example/cpp03/allocation/server.cpp

class server
{
private:
  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;

public:
  server(boost::asio::io_context& io_context, short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) // <------ 
  {
    session_ptr new_session(new session(io_context_));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session_ptr new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
    }
    new_session.reset(new session(io_context_));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }
};

int main(int argc, char* argv[])
{
  try
  {
    // ...
    boost::asio::io_context io_context; // <------ 

    server s(io_context, atoi(argv[1]));

    io_context.run(); // <------ 
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
```

As you can see, such design enforces to use reference in construction like `server`’s constructor.

Since there might be multiple instances of server in one program, this is sound and appropriate. However, `event` is used in system level, and we don’t have to consider the owner of `event` objects because it is always system itself.  
This is why I didn’t designed some type like `event_context`. Therefore, it will be enough to replace `io_context.run()` to `signaled_event_tasks()`.

### Implementation

Each description is based on the actual code. In this note, I will explain with a simplified code. (skip some header, exception spec, etc.)

* [Wrapping `epoll`](#wrapping-epoll)
* [Polling `epoll`](#polling-epoll)
* [Event interface](#event-interface)
* [Event state managment](#events-state-managment)
* [Event await operations](#events-await-operations)

#### Wrapping epoll

* [linux/event_poll.h](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event_poll.h)
* [linux/event_poll.cpp](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event_poll.cpp)

I don't prefer writing wrapper for the system API, but I have another feature(networking) that uses it. Before the start, if you're not familiar with `epoll`, I do recommend you to find some articles and read them first. (I'm sorry!)

> Ok, let me start ...

The wrapper follows RAII and provides some member functions.

* `try_add`: add or modify given `epoll_event` using `epoll_ctl`
* `remove`: `epoll_ctl` with `EPOLL_CTL_DEL`
* `wait`: wait for `epoll_event`s and allows iterate them for each `epoll_wait`

```c++
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

struct event_poll_t final : no_copy_move {
    int epfd;
    const size_t capacity;
    std::unique_ptr<epoll_event[]> events;

  public:
    event_poll_t()  ;
    ~event_poll_t() ;

    void try_add(uint64_t fd, epoll_event& req)  ;
    void remove(uint64_t fd);
    auto wait(int timeout)   -> coro::enumerable<epoll_event>;
};
```

As you can expect, it internally allocates an array to receive events from `epoll_wait`

```c++
event_poll_t::event_poll_t()  
    : epfd{-1},
      // use 2 page for polling
      capacity{2 * getpagesize() / sizeof(epoll_event)},
      events{make_unique<epoll_event[]>(capacity)} {

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0)
        throw system_error{errno, system_category(), "epoll_create1"};
}

event_poll_t::~event_poll_t()  {
    close(epfd);
}
```

With the RAII, `epoll_ctl` can be wrapped with exception throwing code.  
You might be able to write your own version if you hate using the exception.

```c++
void event_poll_t::try_add(uint64_t _fd, epoll_event& req)   {
    int op = EPOLL_CTL_ADD, ec = 0;
TRY_OP:
    ec = epoll_ctl(epfd, op, _fd, &req);
    if (ec == 0)
        return;
    if (errno == EEXIST) {
        op = EPOLL_CTL_MOD; // already exists. try again with mod
        goto TRY_OP;
    }
    // failed
    throw system_error{errno, system_category(), "epoll_ctl"};
}

void event_poll_t::remove(uint64_t _fd) {
    epoll_event req{}; // just prevent non-null input
    auto ec = epoll_ctl(epfd, EPOLL_CTL_DEL, _fd, &req);
    if (ec != 0)
        throw system_error{errno, system_category(), "epoll_ctl EPOLL_CTL_DEL"};
}
```

It's not that complicated :)

#### Polling epoll

* [linux/event_poll.cpp](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event_poll.cpp#L46)

Since `event_poll_t` is internal type, it is free to `co_yield` internal type objects like `epoll_event`. Type for the wait timeout uses `int` instead of `chrono::duration` because [`epoll_wait`](http://man7.org/linux/man-pages/man2/epoll_wait.2.html) allows negative(`-1`) timeout.


```c++
auto event_poll_t::wait(int timeout)  
    -> coro::enumerable<epoll_event> {
    auto count = epoll_wait(epfd, events.get(), capacity, timeout);
    if (count == -1)
        throw system_error{errno, system_category(), "epoll_wait"};

    for (auto i = 0; i < count; ++i) {
        co_yield events[i];
    }
}
```

In another translation unit, [`signaled_event_tasks`](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L97) queries the `event_poll_t` and yield coroutine handles from the user data in `epoll_event`.

```c++
// signaled event list
event_poll_t selist{};

auto signaled_event_tasks()   -> coro::enumerable<event::task> {
    event::task t{}; // it's an alias of `coroutine_handle<void>`

    for (auto e : selist.wait(0)) {
        // we don't care about the internal counter of eventfd.
        //  just receive the coroutine handle
        t = event::task::from_address(e.data.ptr);

        co_yield t;
    }
}
```

Ok, now user code will invoke the function to acquire coroutines with the signaled event. Since its return type is coroutine generator, they can use `for` statement like the following [test code](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/test/c2_concrt_event.cpp#L15).

```c++
TEST_CASE("wait for one event", "[event]") {
  // ...

  for (auto task : signaled_event_tasks()) {
    task.resume();

    // ...
  }

  // ...
}
```

#### Event interface

* [coroutine/concrt.h](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/interface/coroutine/concrt.h#L207)

Now, it's time to implement `event` type. I will write some `private` member functions for each of `await_ready`, `await_suspend`, and `await_resume`.

You may ask why I'm not implementing `await_*` functions directly.
Well, that's because I've met an issue that **exporting `await_*` functions for DLL leads to an internal compiler error**. At least vc140 and vc141 did in my experience.  
My approach is to export those interface functions as `private` and redirecting to them using `public` functions to allow `co_await` statement

```c++
// note:
//    _INTERFACE_ is __declspec(dllexport) or __attribute__((visibility("default")))

class event final : no_copy_move {
  public:
    using task = coroutine_handle<void>;

  private:
    uint64_t state; // <--- will explain in next section

  private:
    _INTERFACE_ void on_suspend(task)  ;
    _INTERFACE_ bool is_ready() const ;
    _INTERFACE_ void on_resume() ;

  public:
    _INTERFACE_ event()  ;
    _INTERFACE_ ~event() ;

    // signal the event object
    _INTERFACE_ void set()  ;

    // ... redirect to private member functions safely ...

    bool await_ready() const  {
        return this->is_ready();
    }
    void await_suspend(coroutine_handle<void> coro)   {
        return this->on_suspend(coro);
    }
    void await_resume()  {
        return this->on_resume();
    }
};

//  Enumerate all suspended coroutines that are waiting for signaled events.
_INTERFACE_
auto signaled_event_tasks()   -> coro::enumerable<event::task>;
```

Its member functions will be explained below. Before that, let's see [how the type implemented constructor and destructor](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L53). It's not complicated!

```c++
event::event() : state{} {
    const auto fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1)
        throw system_error{errno, system_category(), "eventfd"};

    this->state = fd; // start with unsignaled state
}
event::~event()  {
    // if already closed, fd == 0
    if (auto fd = get_eventfd(state))
        close(fd);
}
```

You can see a weird function, `get_eventfd`.

#### Event's state managment

Just like the code above, `event`'s state is from `eventfd` function. However, **we need a piece of information to estimate the event is signaled**.

```c++
class event final : no_copy_move {
  private:
    uint64_t state;
};
```

Instead of using an internal counter of `eventfd`, [I used a bit mask to distinguish that the event object is signaled](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L19). Please follow the comments. I wrote carefully!

```c++
//
//  We are going to combine file descriptor and state bit
//
//  On x86 system,
//    this won't matter since `int` is 32 bit.
//    we can safely use msb for state indicator.
//
//  On x64 system,
//    this might be a hazardous since the value of `eventfd` can be corrupted.
//    **Normally** descriptor in Linux system grows from 3, so it is highly
//    possible to reach system limitation before the value consumes all 63 bit.
//
constexpr uint64_t emask = 1ULL << 63;

//  the msb(most significant bit) will be ...
//   1 if the fd is signaled,
//   0 on the other case
bool is_signaled(uint64_t state)  {
    return emask & state; // msb is 1?
}
int64_t get_eventfd(uint64_t state)  {
    return static_cast<int64_t>(~emask & state);
}
uint64_t make_signaled(int64_t efd)  ;
```

Was the comment enough? With those helper functions, [`set` operation](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L66) becomes really simple.

```c++
void event::set() {
    // already signaled. nothing to do...
    if (is_signaled(state))
        // !!! under the race condition, this check is not safe !!!
        return;

    auto fd = get_eventfd(state);
    state = make_signaled(fd); // if it didn't throwed
                               //  it's signaled state from now
}
```

Let me remind you of one of the requirements.

* For each state, the behavior is like the following
    * Non-signaled:  
      `co_await` will suspend the coroutine 
      and becomes resumable when the event object is signaled.
      If the event is already signaled, the coroutine must not suspend.
    * Signaled:  
      `co_await` won’t suspend
      and the event object becomes non-signaled state after the statement.

Let's just assume that our `event`'s fd is already registered to `epoll`. To make `epoll` return this event in `epoll_wait`, [we will use `write`](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L41). And, of course, bit masking must come after the success of the operation!

```c++
uint64_t make_signaled(int64_t efd)   {
    // signal the eventfd...
    //  the message can be any value
    //  since the purpose of it is to trigger the epoll
    //  we won't care about the internal counter of the eventfd
    auto sz = write(efd, &efd, sizeof(efd));
    if (sz == -1)
        throw system_error{errno, system_category(), "write"};

    return emask | static_cast<uint64_t>(efd);
}
```

Remember that we flagged `EFD_NONBLOCK` for `eventfd` function in the constructor. It was intended :)

#### Event's await operations

The last part of the implementation is for `co_await` statement. `await_ready` and `await_resume` is simple with the masking function.

```c++
bool event::is_ready() const  {
    return is_signaled(state);
}
void event::on_resume()  {
    // make non-signaled state
    this->state = static_cast<decltype(state)>(get_eventfd(state));
}
```

The transition to non-signaled state is based on the requirement. And after `set` member function, `await_ready` will return `true` and will bypass `await_suspend`.

* The event is stateful and has 2 states. 
    * Signaled
    * Non-signaled
* For each state, the behavior is like the following
    * Non-signaled:  
      `co_await` will suspend the coroutine 
      and becomes resumable when the event object is signaled.
      **If the event is already signaled, the coroutine must not suspend**.
    * Signaled:  
      `co_await` won’t suspend
      and **the event object becomes non-signaled state after the statement**.

Now, our keystone function is like this. We declared global variable with `event_poll_t` type when writing `signaled_event_tasks`. [When the coroutine enters `await_suspend`](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L86), we have to send the `coroutine_handle<void>` to the `event_poll_t`.

```c++
// signaled event list.
event_poll_t selist{};

void event::on_suspend(task t) {
    // just care if there was `write` for the eventfd
    //  when it happens, coroutine handle will be forwarded
    //  to `signaled_event_tasks` by epoll
    epoll_event req{};
    req.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
    req.data.ptr = t.address();

    // throws if `epoll_ctl` fails
    selist.try_add(get_eventfd(state), req);
}
```

Here, we use `epoll_event`'s user data to save the coroutine frame's address. Compare the code with the implementation of the `signaled_event_tasks` below. [It constructs coroutine handle from `e.data.ptr`](https://github.com/luncliff/coroutine/blob/ad1e682fe9a01a250e71080366d653af60eda708/modules/linux/event.cpp#L105).

```c++
auto signaled_event_tasks()   -> coro::enumerable<event::task> {
    event::task t{}; // it's an alias of `coroutine_handle<void>`

    for (auto e : selist.wait(0)) {
        // we don't care about the internal counter of eventfd.
        //  just receive the coroutine handle
        t = event::task::from_address(e.data.ptr);

        co_yield t;
    }
}
```

#### Summary for the implementation

It wasn't that hard to combine coroutine with `epoll` and `evnetfd`.  
Let's cover the `event` again.

```c++
// event type uses `eventfd` and bit masking for state check 
class event final : no_copy_move {
  public:
    using task = coroutine_handle<void>; // becomes user data of `epoll_event`

  private:
    uint64_t state; // msb + file descriptor

  public:
    event();      // create fd with `eventfd`
    ~event();     // `close` the fd

    // if it's signaled (msb is 1), no suspend
    bool await_ready(); 

    // bind current fd to epoll 
    //  and its epoll_event will hold the coroutine's handle
    void await_suspend(coroutine_handle<void> coro);

    // make non-signaled (reset msb to 0)
    void await_resume(); 

    // if there is a suspended coroutine,
    //  it means that the event's fd is alreadty registered via `await_suspend`
    //  so we will invoke `write` for the fd.
    //  `epoll` in the `signaled_event_tasks` will report that using `epoll_wait`
    // if it's not suspended (== no waiting coroutine), 
    //  `write` on it won't matter
    void set(); 
};
```

the exported function `signaled_event_tasks` allows user code to acquire suspended(event-waiting) coroutines.  
It might be unsatisfying that those coroutines are not resumed automatically, but if we already have a main loop for event handling, this function can be placed at the point without concerns.

```c++
// access to hidden(global) `epoll` and invoke `epoll_wait`.
//  `epoll_wait` will return `epoll_event`s with `coroutine_handle<void>`
//  this function extracts and yields them to caller
auto signaled_event_tasks()   -> coro::enumerable<event::task>;
```

Whoa, that's all for the implementation details.


### Conclusion

Now we can write a coroutine code like the summary section.  
`epoll` and `eventfd` is simple enough to use, but almost all of their examples use thread(or system process). **With the C++ 20 coroutine, we can use the pair in a more graceful manner**.

```c++
auto wait_for_one_event(event& e, atomic_flag& flag) -> no_return {
  try {

    // resume after the event is signaled ...
    co_await e;

  } catch (system_error& e) {
    // event throws if there was an internal system error
    FAIL(e.what());
  }
  flag.test_and_set();
}

TEST_CASE("wait for one event", "[event]") {
  event e1{};
  atomic_flag flag = ATOMIC_FLAG_INIT;

  wait_for_one_event(e1, flag);
  e1.set();

  auto count = 0;
  for (auto task : signaled_event_tasks()) {
    task.resume();
    ++count;
  }
  // we must enter the loop
  REQUIRE(count > 0);
  // already set by the coroutine `wait_for_one_event`
  REQUIRE(flag.test_and_set() == true);
}
```
