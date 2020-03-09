# Russian Roulette and C++ Coroutines

[한국어](./russian-roulette-kor.md)

In this post, I'd like to show an example which separates definition of a behavior and its execution.

For the example, the Russian Roulette is used.
Following its famous rule, the players' behavior are defined using the coroutine and a game with those players will written be written in an ordinary subroutine.

### Making a revolver 1: Chamber/Trigger

Let's start with makeing a revolver. Since we don't know how many players are going to join, `uint32_t` will be used to define the value space for the chamber.
And we need an additional function, `select_chamber`, to select one chamber value.

```c++
#include <random>

//  for this example,
//  chamber_t is the indices of the revolver's chambers
using chamber_t = uint32_t;

auto select_chamber() -> chamber_t {
    std::random_device device{};
    std::mt19937_64 gen{device()};
    return static_cast<chamber_t>(gen());
}
```

This is indeed the most simple implementation. It might be more complex for the real case, but let's use it anyway. 

We need a trigger now. Of course it can be `pull`ed, and it knows which chamber is `loaded` and which chamber is going to be fired for the moment(`current`).
After pull operation, simply decreasing 1 from the `chamber_t`'s value. Remind that we are going to use a revolver!

```c++
//  trigger fires the bullet
class trigger_t {
  protected:
    const chamber_t& loaded;
    chamber_t current;

  public:
    trigger_t(const chamber_t& _loaded, chamber_t _current)
        : loaded{_loaded}, current{_current} {
    }

  private:
    bool pull() { // pull the trigger. is it the bad case?
        return --current == loaded;
    }

    // ...
};
```

### Defining the players' behavior 

In addition to that, let's make the trigger to an awaitable type.
So all players can **wait** for the bullet after pulling it.

```c++
//  trigger fires the bullet
//  all players will 'wait' for it
class trigger_t {
    // ...
  public:
    bool await_ready() {
        return false;
    }
    void await_suspend(coroutine_handle<void>) {
    }
    bool await_resume() {
        return pull();
    }
};
```

By doing so, each player will be in one of the 3 state.

* Pulled the trigger, and not fired. (Alive!)
* Pulled the trigger, and fired. (Hold X to pay respect)
* Waiting for its turn, and another player received the bullet.

In the 'code' form, it will be like the following.  

`index` is for distinguishing each `player` instance. `fired` should be non-local since the other player will monitor its value.
Also `trigger` is passed by reference because all players share 1 revolver.

```c++
//  this player will ...
//  1. be bypassed
//     (fired = false; then return)
//  2. receive the bullet
//     (fired = true; then return)
//  3. be skipped because of the other player became a victim
//     (destroyed when it is suspended - no output)
auto player(gsl::index id, bool& fired, trigger_t& trigger) -> user_behavior_t {
    // bang !
    fired = co_await trigger;
    fired ? printf("player %zu dead  :( \n", id)
          : printf("player %zu alive :) \n", id);
}
```

What a short function!

Remind that `trigger_t` is an awaitable type. So each player will **suspend** immediately after pulling the trigger. After all players become suspended, then we will **resume** them one by one. 
When a player is resumed, it will check the return value of the `await_resume` and notify the resule(`fired`) by storing the result.
Of course the `player` behavior branches with it.

### Making a revolver 2

Let me explain about the `user_behavior_t` after finishing the definition of the revolver and the game.
1 revolver owns 1 trigger. The point will be expressed through the implementation inheritance. In addition to that, `revolver_t` must be aware of the `loaded` chamber. 

```c++
// revolver knows which is the loaded chamber
class revolver_t : public trigger_t {
    const chamber_t loaded;

  public:
    revolver_t(chamber_t current, chamber_t num_player)
        : trigger_t{loaded, num_player}, //
          loaded{current % num_player} {
    }
};
```

Because someone must **win** the prize, we didn't forget to apply modular arithmetic.

### Progress of the game

Now it's time to define a execution of the game. I already told you it's an ordinay subroutine.
Assume that we have 6 players for 1 game.

```c++
#include <array>

int main(int, char* []) {
    // select some chamber with the users
    array<user_behavior_t, 6> users{};
    revolver_t revolver{select_chamber(),
                        static_cast<chamber_t>(users.max_size())};

    russian_roulette(revolver, users);
    return EXIT_SUCCESS;
}
```

The game, `russian_roulette` is invoked with the resources.
You may not aware of the the GSL(C++ Core **G**uideline **S**upport **L**ibrary), but don't take it so hard.
`gsl::span` is a pair of the pointer and length to support range-`for` statement, `gsl::finally` invokes a given funcion object in its destuction phase. Here, by using it we can guarantee clean up of the coroutine frame.

```c++
#include <gsl/gsl>

// the game will go on until the revolver fires its bullet
auto russian_roulette(revolver_t& revolver, gsl::span<user_behavior_t> users) {
    bool fired = false;

    // spawn player coroutines with their id
    gsl::index id{};
    for (auto& user : users)
        user = player(++id, fired, revolver);

    // cleanup the game on return
    auto on_finish = gsl::finally([users] {
        for (coroutine_handle<void>& frame : users)
            frame.destroy();
    });

    // until there is a victim ...
    for (id = 0u; fired == false; id = (id + 1) % users.size()) {
        // continue the users' behavior in round-robin manner
        coroutine_handle<void>& task = users[id];
        if (task.done() == false)
            task.resume();
    }
}
```

Like I said all `player`s share `fired` variable and `revolver` instance.
Their behavior is defined using the coroutine.
`russian_roulette` subroutine becomes the moderator of the game with the control flow. It's task is to continue this turn-based game by resuming each player coroutine.
When they are resumed through `task.resume()` first time, they will pull the trigger and suspend again.

The game continues (by resume) until `fired` becomes `true`.

When there is a victim, the subroutine will return and 
`on_finish` object will destroy all players' coroutine frame. Simply using `frame.destroy()` will do the work.

### Return type for the player coroutine

In C++ 20 Coroutines, the return type of the coroutine must fulfill the Coroutine Promise Requirement. `user_behavior_t` is such kind of the type.

>
> You can see the [Lewiss Baker's post about it](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type)
>

The first thing we have to do is to define `user_behavior_t::promise_type` which manages the coroutine frame.
We can use `coroutine_traits<user_behavior_t, ...>::promise_type` to do the work, but let's go through the easy way :)

```c++
#include <experimental/coroutine>

using namespace std;
using namespace std::experimental;

class promise_manual_control {
  public:
    auto initial_suspend() {
        return suspend_always{}; // suspend after invoke
    }
    auto final_suspend() {
        return suspend_always{}; // suspend after return
    }
    void unhandled_exception() {
        // this example never 'throw'. so nothing to do
    }
};
```

Each function's role is like the following.

* `initial_suspend`: Decide whether to suspend after the coroutine functions is invoked. At this moment the coroutine's frame is allocated.
* `final_suspend`: Decide whether to suspend after the coroutine function is returned(`co_return`).
* `unhandled_exception`: Invoked when an exception is thrown in the coroutine function's body

Returning `suspend_always` type means that the coroutine is willing to suspend for the defined moment. If you don't want to suspend and continue the flow, you should return `suspend_never`.  
For this example, `player` coroutine's creation, progress(suspend/resume), destruction 
is fully managed by the subroutine `russian_roulette`. 
So we will return `suspend_always` type and forget about the other concerns.

### Exposing the player coroutine's frame

`user_behavior_t::promise_type` inherits `promise_manual_control` and add 2 funtion to meet the requirement.

* `return_void`: invoked just after `co_return`
* `get_return_object`: the function's return becomes the return type of the coroutine function. Here, we will return `user_behavior_t` directly.

```c++
//  behavior will be defined as a coroutine
class user_behavior_t : public coroutine_handle<void> {
  public:
    class promise_type : public promise_manual_control {
      public:
        void return_void() {
        }
        auto get_return_object() -> user_behavior_t {
            return {this};
        }
    };

  private:
    user_behavior_t(promise_type* p) : coroutine_handle<void>{} {
        coroutine_handle<void>& self = *this;
        self = coroutine_handle<promise_type>::from_promise(*p);
    }

  public:
    user_behavior_t() = default;
};
```

The inheritance is `public`. 
So it exposes those member functions which controls the coroutine. (`resume`, `done`, and `destroy`)

After its base class construction, `coroutine_handle<void>` is in the empty state.
Here, I used the simplest way to **translate** `promise_type` to its matching `coroutine_handle<void>`. 
You can see that `from_promise` of the `coroutine_handle<promise_type>` is doing the work.

Unless we trust the execution manager, `russian_roulette`, such exposure won't matter.
Remind that `coroutine_handle<void>` follows the semantics of `void*`.
So if you want to prevent some mistakes for future extenstion, 
you had better define or `delete` copy/move functions and its destructor.

### Conclusion

[You can run the all-in-one code with the Compiler Explorer.](https://godbolt.org/z/rlKE7f)

Like https://github.com/Naios/continuable, C++ Coroutines can be used like a sugar for the `future<T>`. 
But that's the not only usage.

Since the coroutine frame contains an index to distinguish its suspension points, 
we can't tell it is totaly different from the state pattern.
We need to define awaitable type and the return types for our logic, 
but by doing so the point of suspension and continuation can be written more naturally.

For my perspective this gives more resilience to the code.
