//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <array>
#include <random>

#include <gsl/gsl>

#include <coroutine/return.h>

using namespace std;

class promise_manual_control {
  public:
    auto initial_suspend() {
        return suspend_always{}; // suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_always{}; // suspend after return
    }
    void unhandled_exception() {
        // this example never 'throw'. so nothing to do
    }
};

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

//  for this example,
//  chamber is an indices of the revolver's cylinder
using chamber_t = uint32_t;

auto select_chamber() -> chamber_t {
    std::random_device device{};
    std::mt19937_64 gen{device()};
    return static_cast<chamber_t>(gen());
}

//  trigger fires the bullet
//  all players will 'wait' for it
class trigger_t {
    const chamber_t& loaded;
    chamber_t& current;

  public:
    trigger_t(const chamber_t& _loaded, chamber_t& _current)
        : loaded{_loaded}, current{_current} {
    }

  private:
    bool pull() { // pull the trigger. is it the bad case?
        return --current == loaded;
    }

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

// revolver knows which is the loaded chamber
class revolver_t : public trigger_t {
    const chamber_t loaded;
    chamber_t current;

  public:
    revolver_t(chamber_t chamber, chamber_t num_player)
        : trigger_t{loaded, current}, //
          loaded{chamber % num_player}, current{num_player} {
    }
};

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

int main(int, char* []) {
    // select some chamber with the users
    array<user_behavior_t, 6> users{};
    revolver_t revolver{select_chamber(),
                        static_cast<chamber_t>(users.max_size())};

    russian_roulette(revolver, users);
    return EXIT_SUCCESS;
}
