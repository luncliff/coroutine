# Russian Roulette and C++ Coroutines

[English](./russian-roulette.md)

이번에는 여러 코루틴은 동작에 집중하고 이들의 진행을 하나의 서브루틴에서 수행하는 코드를 하나 작성해보고자 합니다.
예시로는 굉장히 유명한 규칙을 가진 러시안 룰렛과 그 게임의 참가자들을 사용해 봤습니다.

### 리볼버 준비하기 1: 약실/방아쇠

우선 리볼버를 만드는 것부터 시작해보겠습니다. 참가 인원이 얼마나 있을지 모르기 때문에, `uint32_t`를 사용해 충분히 많은 약실(chamber)을 준비할 겁니다.
그리고 임의의 약실을 선택하는 `select_chamber`를 아래와 같이 정의하겠습니다.

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

경우에 따라서는 인자를 받아 상황에 따라 약실을 선택할지 모르지만, 이 예시에서는 가장 단순한 방법(무작위)을 사용하겠습니다.

약실이 있으니 이제 방아쇠(trigger)를 준비해야겠군요.
방아쇠는 당길(`pull`) 수 있으며, 현재 공이가 맞물린 약실(`current`)과 장전된 약실(`loaded`)에 대해서 알고 있습니다.
한번 방아쇠를 당기면 약실이 회전할테니, 여기서는 간단히 `chamber_t`의 값을 1 감소 시키는 것으로 약실을 회전시키겠습니다.

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

### 참가자 행동 정의하기

여기서 방아쇠는 Awaitable 타입으로 정의해서, 게임 참가자들이 방아쇠를 당긴 후 결과를 **기다리도록** 만들겠습니다.

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

이렇게 정의한 이유는 참가자(player)들이 3가지 상태를 가지기 때문입니다.

* 방아쇠를 당겼지만, 총이 발사되지 않았다 (살았다!)
* 방아쇠를 당겨서, 총이 발사되었다 (X를 눌러 조의를 표하십시오.)
* 차례를 기다리던 중 다른 사람이 이미 총알을 받았다

코드로 표현하자면 아래와 같습니다.
`index`를 사용해 각 `player`들을 구분하고, 코루틴 외부에 위치한 `fired`(불이야!)의 값을 변경하는 방법으로 총알이 발사되었을 때 다른 코루틴들이 이를 확인할 수 있도록 하겠습니다.
모두가 같은 리볼버의 방아쇠를 사용하므로, `trigger`는 참조를 사용하였습니다.

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

이렇게 하면 실제로는 모든 참가자 코루틴들은 방아쇠를 미리 당겨놓고 기다리게(suspend) 됩니다. 그리고 방아쇠를 한번씩 당기면서 차례대로 깨워보는(resume) 것이죠.
차례가 되면(resumed) `trigger_t`은 `await_resume`의 반환값을 통해 결과(`fired`)를 전달할 것이고 그 값에 따라서 `player`의 동작이 달라지게 됩니다.

### 리볼버 준비하기 2

이제 리볼버를 마저 완성하고, `user_behavior_t`에 대해 설명하겠습니다. 이미 위에서 `trigger_t`를 정의 했으므로, `revolver_t`는 1개의 방아쇠와 장전된 약실(`loaded`)에 대한 정보를 가지고 있어야 합니다.

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

물론 누군가는 **당첨**되어야 하기 때문에, 모듈로 연산을 하는 것도 잊지 않았습니다.

### 게임 진행과정

이제 참가자와 도구(리볼버)가 모두 정의되었으니, 게임 진행과정을 표현해보겠습니다.
이 게임에서는 총 6명의 참가자가 있다고 가정하겠습니다.

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

참가자와 도구가 준비되면, `russian_roulette`을 호출하여 게임을 시작합니다.
GSL(C++ 핵심 가이드라인 지원 라이브러리)에 익숙하지 않으시더라도 읽는데 문제는 없을 것입니다.  
모르는 분들을 위해 설명하자면, `gsl::span`은 범위를 표현하여 range-`for` 구문을 사용할 수 있게 해주고, `gsl::finally`는 개체가 소멸하는 시점에 함수를 호출하는 기능을 합니다.

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

모든 `player`들이 `fired`와 `revolver`를 공유하며 게임을 시작합니다. 이들의 행동은 코루틴으로 정의하였습니다.
참가자들의 수와 무관하게, `russian_roulette` 서브루틴은 게임의 진행자가 되어 코루틴들이 차례대로 방아쇠를 당기게 만들것입니다. 이는 `task.resume()`을 통해서 이루어집니다. 

게임은 `fired`가 `true`가 될 때까지 계속됩니다.

게임이 끝나면 `on_finish` 개체가 모든 참가자들이 사용한 메모리(코루틴 프레임)를 회수할 것입니다. 간단히 `frame.destroy()`하는 것으로 이를 처리할 수 있습니다.

### Player 코루틴을 위한 반환 타입

C++ 20 Coroutines에서 코루틴을 정의하려면, Coroutine Promise Requirement를 만족하는 반환 타입을 가져야 합니다. 이 조건에 맞춰서 `user_behavior_t`를 정의해보겠습니다.

>
> Promise에 대한 상세한 설명은 [Lewiss Baker가 다룬 바 있습니다](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type).  
> 한글 자료는 [C++ Korea 5회 세미나의 발표자료](https://github.com/CppKorea/CppKoreaSeminar5th/blob/master/5%20-%20C%2B%2B%20Coroutine%20%EC%95%8C%EC%95%84%EB%B3%B4%EA%B8%B0%3B%20%EC%A0%91%EA%B7%BC%EB%B2%95%2C%20%EC%BB%B4%ED%8C%8C%EC%9D%BC%EB%9F%AC%2C%20%EA%B7%B8%EB%A6%AC%EA%B3%A0%20%EC%9D%B4%EC%8A%88/%5BC%2B%2B%20Korea%205th%20Seminar%5D%20C%2B%2B%20Coroutine%20%EC%95%8C%EC%95%84%EB%B3%B4%EA%B8%B0%3B%20%EC%A0%91%EA%B7%BC%EB%B2%95%2C%20%EC%BB%B4%ED%8C%8C%EC%9D%BC%EB%9F%AC%2C%20%EA%B7%B8%EB%A6%AC%EA%B3%A0%20%EC%9D%B4%EC%8A%88%EB%93%A4.pdf)를 참고해주시길 바랍니다. 
>

우선 코루틴의 프레임을 관리하는 `user_behavior_t::promise_type`을 정의해야 합니다. 엄밀하게는 `coroutine_traits<user_behavior_t, ...>::promise_type` 이지만, 여기서는 가장 간단한 방법을 사용하겠습니다.

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

각 함수들은 다음과 같은 역할을 합니다.

* `initial_suspend`: 코루틴이 호출(invoke)되어 해당 코루틴의 프레임이 생성된 직후 중단 여부를 결정
* `final_suspend`: 코루틴이 반환(`co_return`)된 이후 해당 코루틴의 중단 여부를 결정
* `unhandled_exception`: 코루틴 내에서 처리되지 못한 Exception이 발생할때 호출

`suspend_always` 타입 개체를 반환하는 행위는 단순히 현재 코루틴을 중단하겠다는 의미입니다. 만약 중단하지 않고 바로 진행한다면 `suspend_never`를 반환하면 됩니다.  
위에서 작성한 예시에서 `player` 코루틴들의 진행(suspend, resume) 여부는 모두 게임의 진행자인 `russian_roulette` 서브루틴에서 관리하므로 두 경우 모두 `suspend_always`를 반환하도록 하였습니다.

### Player 코루틴 프레임에 대한 접근

`user_behavior_t::promise_type`는 `promise_manual_control`를 상속받으면서, 2가지 멤버 함수를 정의합니다.

* `return_void`: `co_return`을 처리합니다
* `get_return_object`: 코루틴의 반환타입을 생성할 수 있도록 임의의 개체를 반환합니다. 이 경우는 `user_behavior_t`를 바로 반환하도록 하였습니다.

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

`user_behavior_t`는 `coroutine_handle<void>`를 상속 받아 `resume`, `done`, `destroy`와 같이 코루틴을 제어할 수 있도록 하였습니다.
이를 위해서는 `promise_type`으로부터 `coroutine_handle<void>`를 얻을 방법이 필요한데, 이는 `coroutine_handle<promise_type>`을 사용하여 해결할 수 있습니다.

제어는 `russian_roulette`에서 직접 이루어지므로 지금과 같은 코드로도 문제가 없습니다. 하지만 `coroutine_handle<void>`는 마치 `void*`처럼 사용할 수 있기 때문에, 부주의한 실수를 예방하려면 복사/이동 생성자, 복사/이동 대입 연산자, 소멸자를 추가로 정의해야 할 것입니다.

### 결론

[All-in-one 코드는 Compiler Explorer에서 실행해볼 수 있습니다.](https://godbolt.org/z/rlKE7f)

https://github.com/Naios/continuable 처럼 `future<T>`를 더 편리하게 사용하기 위해 코루틴을 사용할수도 있지만, 이는 한가지 방법일 뿐입니다.

코루틴 프레임 안에 중단 지점에 대한 정보(index)가 있기 때문에 구현레벨에서는 상태 패턴과 크게 다르지 않습니다.
다만 차이점이 있다면 상태 패턴이 추가적인 타입을 작성해야 하지만, 코루틴에서는 중단(suspend)과 재개(resume)를 자연스럽게 기술하는데 좀 더 초점을 둘 수 있죠.
이는 중단과 재개하는 과정이 코드의 구조에 영향을 주는 일이 줄어든다고 요약할 수 있겠습니다.
