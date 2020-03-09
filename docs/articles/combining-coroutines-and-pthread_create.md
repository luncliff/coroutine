
# Combining C++ Coroutines and `pthread_create`

> commit: [`93ed8748`](https://github.com/luncliff/coroutine/tree/93ed8748a4cb70c73ee92892ab4161b7985d5572)

Some system functions use callback and an argument in `void*` type.  
In this article, I'm going to cover how such functions can be wrapped with `co_await`

The base of examples here is my experience to try something like [CppCon 2016 : Kenny Kerr & James McNellis "Putting Coroutines to Work with the Windows Runtime"](https://www.youtube.com/watch?v=v0SjumbIips) with the POSIX threads.
If you are a beginner, please do read [the article about `promise_type` by Lewiss Baker](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type).
The article will enhance your understanding greatly.

## Creating a new thread

### Design

What I'm going to try is creating a new thread in a simple manner.
Traditionally we had to separate the step before/after `pthread_create`. 
To do so, we must write an additional function which is executed on the new thread.
[Like the `thread_start` in the `pthread_create` manual](http://man7.org/linux/man-pages/man3/pthread_create.3.html#EXAMPLE).


But `co_await` can glue two routines into one.
Let's see how it looks like.

```c++
void per_thread_task();

auto work_on_new_thread(pthread_t& tid, const pthread_attr_t* attr)
    -> no_return {

    // this line is executed on the spawner thread
    co_await spawn_pthread(tid, attr);

    // the function is executed on the pthread with 'tid'
    per_thread_task();
}
```
Since it's an example, it will be differenct when you have to write for your own.
What I want to show here is that the look & feel of the wrapping pattern.

### Implementation

#### `start_routine` to resume the coroutine

Obviously the most important functions are the life cycle functions.

* [`pthread_create`](http://man7.org/linux/man-pages/man3/pthread_create.3.html)
* [`pthread_join`](http://man7.org/linux/man-pages/man3/pthread_join.3.html)

```c++
#include <pthread.h>

// create a new thread
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);

// join with a terminated thread
int pthread_join(pthread_t thread, void **retval);
```

Here, we can control 2 parameters. 

* `start_routine` to designate a routine for the thread
* `arg` to forward an argument to the routine

With them we can imagine some code in the system will be like this.

```c++
void pthread_procedure(void* out, ...){
    // ... PC(Program Counter) jumps to here ...
    out = start_routine(arg);
    // perform `pthread_exit` ...
    // deliver 'out' to `pthread_join` or somewhere ...
}
```

We will define a simple function for the `start_routine`. 
It constructs the `coroutine_handle` from the `arg` and then resume it after checking with `done`.

```c++
void* resume_on_pthread(void* ptr){
    // assume we will receive an address of the coroutine frame
    auto task = coroutine_handle<void>::from_address(ptr);

    if(task.done() == false)
        task.resume();

    return task.address();  // coroutine_handle<void>::address();
}
```

#### Forwarding `arg` from `co_await`

So to make the function work properly, we have to forward `coroutine_handle<void>` as an argument of `arg`.
This is easy since there is a member fuction, `address`.

`await_suspend` will give us the handle like the following code.

```c++
struct pthread_spawner_t {
    constexpr bool await_ready() const noexcept {
        return false; // always false to utilize `await_suspend`
    }
    void await_suspend(coroutine_handle<void> rh) noexcept(false) {
        pthread_t tid{};
        pthread_create(&tid, nullptr, 
                       resume_on_pthread, rh.address());
        // skipped error check for simplicity :)
    }
    void await_resume() noexcept {
        // this function will in the call stack of `resume_on_pthread` ...
    }
};
```

Or if you like the simplicity, you can inherit `suspend_always` in the `std::experimental`.

```c++
#include <experimental/coroutine>
using std::experimental::suspend_always;

struct pthread_spawner_t : public suspend_always {
    // hide the function of `suspend_always` ...
    void await_suspend(coroutine_handle<void> rh) noexcept(false) {
        pthread_t tid{};
        if (auto ec = pthread_create(&tid, nullptr, 
                                     resume_on_pthread, rh.address()))
            throw system_error{ec, system_category(), "pthread_create"};
    }
};
```

In the meantime, now we have an awaitable type that creates a new thread.
So we can trigger it in 1 line.

```c++
auto work_on_new_thread() -> no_return {
    co_await pthread_spawner_t{};
    // ... 
}
```

### Concerns

You might curious about `throw` in the second `pthread_spawner_t::await_suspend`.  
In general, the function is the last point for the error handling.
For our case, suppose that `pthread_create` failed. The callback `resume_on_pthread` won't be invoked.

That means the coroutine's frame can't be `resume`d and coverage after `co_await` is going to be lost.
So we have to make caller handle the situation **before** the coroutine becomes suspended state. One of the way is to throw an exception in `await_suspend`.


## Dealing with the thread's join

### Design

Now we have to support join for the thread. `pthread_join` is so simple that we can't misuse it.
What we have care is not the function but the behavior after `pthread_join` returned without an error.

Do you remember that I returned the coroutine frame's address for our `start_routine`?
```c++
void* resume_on_pthread(void* ptr){
    auto task = coroutine_handle<void>::from_address(ptr);
    // ...
    return task.address();
}
```

So in somewhere we have to get the address and `destroy` it.

```c++
auto join_and_destroy(pthread_t tid){
    void* ptr{};
    if(auto ec = pthread_join(tid, &ptr))
        throw system_error{ec, system_category(), "pthread_join"};
    
    auto coro = coroutine_handle<void>::from_address(ptr);
    if(coro)
        coro.destroy();
}
```

But this code is dangerous.
The reason is that there is no way to know whether the frame is not destroyed.

When the frame is `final_suspend`ed, we can check it is `done() == true` and then `resume`/`destroy` it.
But when the corouitine didn't suspended after `final_suspend`, its `co_return` will destroy the frame immediately.
So the `coro.destroy()` above is a double-delete situation.

Here, our plan is to provide a special return type which guarantees safe destruction of the frame.

### Implementation


#### Intend of the final suspension

A coroutine's frame is preserved if its `coroutine_traits<R>::promise_type` does final suspension.
That means, it intended manual destruction of the frame, like `generator<T>`.

```c++
// From VC++, <experimental/generator>
namespace experimental {
	template <typename _Ty, typename _Alloc = allocator<char>>
	struct generator
	{
		struct promise_type
		{
			_Ty const *_CurrentValue;
            // ...
			bool final_suspend()
			{
				return (true);
			}
            // ...
        };
        // ...
		~generator()
		{
			if (_Coro) { // manual destruction of 
                         // the generator coroutine's frame
				_Coro.destroy();
			}
		}
	private:
		coroutine_handle<promise_type> _Coro = nullptr;
    };

} // namespace experimental
```

You can see `destroy()` is invoked in its destructor.
We will follow the way.

#### Preserving frame for `pthread_join`

There are 2 things that should be guaranteed.

* Meet the condition `final_suspend == true` so `pthread_join` can receive the **destroyable** frame
* Perform `destroy` after `pthread_join`


Here is the type. 
It will be a return type for the coroutine which `co_await`s `pthread_spawner_t`.

```c++
class pthread_joiner_t final {
  public:
    class promise_type final {
      public:
        auto initial_suspend(){ return suspend_never{}; }
        auto final_suspend(){
            // preserve the frame after `co_return`
            return suspend_always{};
        }
        void return_void(){
            // we already returns coroutine's frame.
            // so `co_return` can't have its operand
        }
        void unhandled_exception(){
            // ...
        }
        auto get_return_object() -> promise_type*{
            return this;
        }
    };

  private:
    pthread_t tid;

  public:
    pthread_joiner_t(promise_type*) : tid{} {
    }
    ~pthread_joiner_t() noexcept(false) { 
        void* ptr{};
        // we must acquire valid `tid` before the destruction
        if(int ec = pthread_join(tid, &ptr)){
            throw system_error{ec, system_category(), "pthread_join"};
        }
        if(auto frame = coroutine_handle<void>::from_address(ptr)){
            frame.destroy();
        }
    }
};
```

So if we use it as a return for the coroutine function, it will be like the following.
Review the look and feel with the `pthread_spawner_t`.

```c++
auto resume_on_new_thread(const pthread_attr_t* attr) -> pthread_joiner_t {
    co_await pthread_spawner_t{attr};
    // ...
}

void owner_subroutine(){
    // A wrapper subroutine to guarantee join.
    auto joiner = resume_on_new_thread(nullptr);

    // It can be useful if the `joiner` supports `operator pthread_t()`
    // pthread_t worker_id = joiner;
}
```

It seems like we are almost done, but it's not.

### Concerns

You may ask that such design is seriously vulnerable when `pthread_exit` is used explicitly.
My answer for that is, most of `pthread_exit`'s usage is careful enough.
You can note for the dangerous point in your comment, document, or even `#warn` for the issue.
And the users are able to redesign/rewrite their own logic with the note.

## Context-Aware `co_await` through `await_transform`

### Design

```c++
auto resume_on_new_thread(const pthread_attr_t* attr) -> pthread_joiner_t {
    co_await pthread_spawner_t{attr};
    // ...
}

void owner_subroutine(){
    // A wrapper subroutine to guarantee join.
    auto joiner = resume_on_new_thread(nullptr);
}
```

This example looks fine, but it will be more safe when `pthread_spawner_t` is used **if and only if** the return type is `pthread_joiner_t`.
Fortuantely C++ Coroutines specifies `await_transform` which can do the work.

So our final goal will be like this.
Can you find the difference?

```c++
auto resume_on_new_thread(const pthread_attr_t* attr) -> pthread_joiner_t {
    co_await attr;
    // ...
}
```

`pthread_attr_t` can't work alone since it's a reserved type for the `pthread_create`.
So when there is an instance of it, there must be an **intent** to create a new thread with it.
Therefore `co_await` on it does express something like "wait for the thread creation and then ...".

### Implementation

#### Pairing `pthread_joiner_t` with `pthread_spawner_t`

Until now, `pthread_spawner_t` was an global type that can be used anywhere.
The first we have to do is to make it available **only** when `pthread_joiner_t` is known.
Simply nesting it will do the work.

```c++
class pthread_joiner_t final {
  public:
    class promise_type;

    // nest(hide) the `pthread_spawner_t` and allow access to `promise_type`
    class pthread_spawner_t final {
        friend class promise_type;

      public: // awaitable interface must be open to public
        constexpr bool await_ready() const noexcept {
            return false;
        }
        void await_suspend(coroutine_handle<void> rh) noexcept(false) {
            // invoke `pthread_create` ...
        }
        void await_resume() noexcept {
            // nothing to do ...
        }

      private:
        explicit pthread_spawner_t(const pthread_attr_t* _attr) 
            : attr{_attr}{}

        const pthread_attr_t* const attr;
    };

  private:
    pthread_t tid;
};
```

#### Defining `await_transform`

As mentioned above, we will define the `await_transform` in `pthread_joiner_t::promise_type`.
The role of the function is to transform the operand of `co_await` to an awaitable type.
In this case, the awaitable type is `pthread_spawner_t`.

```c++
class pthread_joiner_t final {
  public:
    class promise_type;
    class pthread_spawner_t final {
        friend class promise_type;
        // ...
      private:
        explicit pthread_spawner_t(const pthread_attr_t* _attr) 
            : attr{_attr}{}

        const pthread_attr_t* const attr;
    };

    class promise_type final {
        // ... see 'Preserving frame for `pthread_join`' ...

        // We can consider the `pthread_attr_t*` as an intent to create a new pthread.
        // Therefore, `promise_type` will transform the type to `pthread_spawner_t`
        auto await_transform(const pthread_attr_t* attr){
            return pthread_spawner_t{attr}
        }
    };
    // ... same with above ...
  private:
    pthread_t tid;
};
```

By doing so we can achieve our design goal.
Because typical return types for the coroutines are not aware of how to `co_await` on `pthread_attr_t`.

#### Exposing `pthread_t`

Now the last part is to store the new pthread's id in the `pthread_joiner_t`.
This is what I wrote above.

```c++
class pthread_joiner_t final {
  public:
    class promise_type;
    class pthread_spawner_t;

  private:
    pthread_t tid;

  public:
    // ...
    ~pthread_joiner_t() noexcept(false) { 
        void* ptr{};
        // we must acquire valid `tid` before the destruction
        if(int ec = pthread_join(tid, &ptr)){
            throw system_error{ec, system_category(), "pthread_join"};
        }
        if(auto frame = coroutine_handle<void>::from_address(ptr)){
            frame.destroy();
        }
    }
};
```

However, we call `pthread_create` in `pthread_spawner_t`. 
So we can say there must be some sharing point to get the valid `pthread_t` from `pthread_joiner_t`.

```c++
class pthread_joiner_t final {
  public:
    class pthread_spawner_t final {
      public:
        // ...
        void await_suspend(coroutine_handle<void> rh) noexcept(false) {
            pthread_t tid{};
            if (auto ec = pthread_create(&tid, nullptr, 
                                        resume_on_pthread, rh.address())){
                throw system_error{ec, system_category(), "pthread_create"};
            }
        }
      private:
        explicit pthread_spawner_t(const pthread_attr_t* _attr) 
            : attr{_attr}{}

        const pthread_attr_t* const attr;
    };
  private:
    pthread_t tid; // <--- duplicate with that of the `pthread_spawner_t`
  public:
    // ...
    ~pthread_joiner_t() noexcept(false);
};
```

At this moment, we must decide where to place the therad's id.
In this code `pthread_joiner_t` is holding it as a member variable.
That's pretty general for most of type **for subroutines**.

Think of the implementation that `pthread_spawner_t` sends thread's id to `pthread_joiner_t`.
If you decide to do so, you have to write a coroutine that aware of it's activator's address.
To be more precise, the address of its return type's object.

```c++
auto resume_on_new_thread(const pthread_attr_t* attr) 
    -> pthread_joiner_t 
{
    // Can we know the address of the returned pthread_joiner_t object ? 
    // Even if that is possible, using the address is a sound design ? 
}
```

Definatly that will lead to the bad(compex) code.

Remember that we are preserving the coroutine frame until `pthread_join`.
In other words, it is a valid behavior to access the frame **before** we invoke `pthread_join`.
With the point, let's save the thread's id in the coroutine's frame, and access to the memory location when the thread's id is needed.

Placing the object in the coroutine's frame isn't that hard.
[Since the `promise_type` object is placed in the frame](https://github.com/luncliff/coroutine/pull/9), accessing to it can be done with 1 pointer.
I already showed you how to get a ponter to `promise_type`. 
Let me show you again.

```c++
class pthread_joiner_t final {
  public:
    class promise_type;
    class pthread_spawner_t;

    class promise_type final {
        auto initial_suspend(){ return suspend_never{}; }
        auto final_suspend(){
            return suspend_always{};
        }
        void unhandled_exception(){
        }
        auto await_transform(const pthread_attr_t* attr){
            return pthread_spawner_t{attr};
        }
        auto get_return_object() -> promise_type*{
            return this;
        }
      public:
        pthread_t tid{};
    };
  public:
    // we will receive the pointer from `get_return_object`
    pthread_joiner_t(promise_type* p) : promise{p} {
    }
    ~pthread_joiner_t() noexcept(false) { 
        // ... join with the thread id ...
        // ... destroy the coroutine frame ...
    }

    // and we can access to the `tid` through the pointer
    operator pthread_t() const noexcept {
        return promise->tid;
    }
  private:
    promise_type* const promise;
};
```

So the last job is to update `tid` in the `promise_type`. 
Let's rewrite the `pthread_spawner_t`.

```c++
class pthread_joiner_t final {
  public:
    class promise_type;

    class pthread_spawner_t final {
        friend class promise_type;

      public:
        constexpr bool await_ready() const noexcept {
            return false;
        }
        void await_suspend(coroutine_handle<void> rh) noexcept(false) {
            pthread_create(this->tid, this->attr, 
                           resume_on_pthread, rh.address());
            // skipped error check for simplicity :)
        }
        void await_resume() noexcept {
            // ... do nothing ...
        }

      private:
        // receives 2 pointer at once
        pthread_spawner_t(pthread_t* _tid, const pthread_attr_t* _attr) 
            : tid{_tid}, attr{_attr}{
        }

        pthread_t* const tid; // pointer to the memory location in the promise
        const pthread_attr_t* const attr;
    };

    class promise_type final {
        auto await_transform(const pthread_attr_t* attr){
            // provide the address at this point
            return pthread_spawner_t{addressof(this->tid), attr};
        }
        // ...
        pthread_t tid{};
    };
    // ...
};
```

Whoa, that was a long run !

## Conclusion

You can run the example with the WandBox.  
[https://wandbox.org/permlink/qQ1vbwshsoujYnI0](https://wandbox.org/permlink/qQ1vbwshsoujYnI0)

In this article we covered 2 things by wrapping `pthread_create` for the coroutines step by step.

* How the awaitable can wrap the system function which uses a callback and it has a `void*` parameter
* How to define `await_transform` and which limitation it makes

However, we only wrapped 2 pthread life cycle functions.
So if the coroutine invokes `pthread_exit` instead of `co_return`, 
the assumption of the `pthread_joiner_t` will be broken.

What I didn't cover here will be a good coroutine design practice for you.

* Move/Copy handling of the types
* The detach of a thread
* Triggering **multiple** thread createion for 1 `pthread_joiner_t`

### After Note

[Arthur O'Dwyer](https://quuxplusone.github.io/blog/) let me know how to improve the final(WandBox) example using `private` constructor. I will cover that in later post.  
Appreciate for his help and opinions! :D


### Allowing another awaitable types

You may need to define more generic `await_transform` like [this](https://github.com/luncliff/coroutine/blob/93ed8748a4cb70c73ee92892ab4161b7985d5572/interface/coroutine/thread.h#L167).

```c++
class pthread_joiner_t final {
  public:
    class promise_type final {

        // the original
        auto await_transform(const pthread_attr_t* attr){
            // provide the address at this point
            return pthread_spawner_t{addressof(this->tid), attr};
        }

        // redirect to const pointer
        inline auto await_transform(pthread_attr_t* attr) noexcept(false) {
            return await_transform(static_cast<const pthread_attr_t*>(attr));
        }

        // allow general co_await usage
        template <typename Awaitable>
        decltype(auto) await_transform(Awaitable&& a) noexcept {
            return std::forward<Awaitable&&>(a);
        }
    };
    // ...
};
```

