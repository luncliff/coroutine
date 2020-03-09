
# MSVC Coroutine 알아보기

> Written in 2017/03/02

[English](./exploring-msvc-coroutine.md)

### 참고자료
#### 제안서
 - [N3858](https://isocpp.org/files/papers/N3858.pdf)
 - [N3977](https://isocpp.org/files/papers/N3977.pdf)
 - [N4134](https://isocpp.org/files/papers/N4134.pdf)
 - [N4402](https://isocpp.org/files/papers/N4402.pdf)

#### Visual C++ Team Blog
 - [Coroutines in Visual Studio 2015– Update 1](https://blogs.msdn.microsoft.com/vcblog/2015/11/30/coroutines-in-visual-studio-2015-update-1/)
 - [More about resumable functions in C++](https://blogs.msdn.microsoft.com/vcblog/2015/04/29/more-about-resumable-functions-in-c/)

#### 영상
 - CppCon 2016 : [Gor Nishanov "C++ Coroutines: Under the covers"](https://channel9.msdn.com/events/CPP/CppCon-2016/CppCon-2016-Gor-Nishanov-C-Coroutines-Under-the-covers)
 - CppCon 2016 : [James McNellis "Introduction to C++ Coroutines"](https://channel9.msdn.com/events/CPP/CppCon-2016/CppCon-2016-James-McNellis-Introduction-to-C-Coroutines)
 - CppCon 2016 : [Kenny Kerr & James McNellis "Putting Coroutines to Work with the Windows Runtime"](https://channel9.msdn.com/events/CPP/CppCon-2016/CppCon-2016-Kenny-Kerr--James-McNellis-Putting-Coroutines-to-Work-with-the-Windows-Runtime)
 - CppCon 2016 : [John Bandela “Channels - An alternative to callbacks and futures"](https://www.youtube.com/watch?v=N3CkQu39j5I&t=1257s)
 - CppCon 2015 : [Gor Nishanov "C++ Coroutines - a negative overhead abstraction"](https://channel9.msdn.com/events/CPP/CppCon-2015/CPPConD03V008)
 - Meeting C++ 2015 : [James McNellis "An Introduction to C++ Coroutines"](https://www.youtube.com/watch?v=YYtzQ355_Co&t=2890s)
 - Meeting C++ 2015 : [Grigory Demchenko "Asynchrony and Coroutines"](https://www.youtube.com/watch?v=SbaLI2ZcyY0&t=2180s)
 - CppCon 2014 : [Gor Nishanov "await 2.0: Stackless Resumable Functions"](https://channel9.msdn.com/events/CPP/C-PP-Con-2014/0006-await-20-Stackless-Resumable-Functions)

#### GitHub 저장소
 - [kirkshoop/**await**](https://github.com/kirkshoop/await)

### 주의사항
우선 MSVC의 Coroutine을 사용하려면 3가지가 필요하다.

 - Visual Studio 2015 Up3 혹은 그 이상의 개발환경
 - `/await` : 컴파일 옵션
 - `<experimental/*>` : 소스 코드에 넣을 헤더파일

![Fig: Command line option](https://cqldba.bn1304.livefilestore.com/y4mxWo9KNVrGutUuX0-nCTZefQXnqsDaRe8UNOQ89xEjUO2BrVrUuHz0c5cBAiZilYehHoPUZaJDruvP-6h-Q5hmmnSbwyNzTa2XcCNXa3mLaCCiUZxZIn7t3XBhRbvJXIAh2PSqJ2ZVKx_w6ytO_khp85PfSBYluHbSAUdzVW98b-5E-frvtigKcY3FmDqFBO-KfIGMFdBX2G_ibX9hwyB5Q?width=1024&height=730&cropmode=none)

## 정의
그래서, 코루틴이 대체 뭔가요?

### 고식(古式) : 기계레벨에서 생각하기

기본적으로, 프로시저의 호출은 `goto`라고 할 수 있다. 그런데 어디로 `goto`한다는 것일까? 다음에 수행할 명령어일 것이다. 명령어는 루틴과 프로그램의 일부이기도 한데, 잠시 여기서 한번 짚어보겠다.

[프로그램]은 일련의 [프로그램 상태]들로 볼 수 있다. 그리고 여기서 [프로그램 상태]란, 프로그래머가 정의한 것이어야 하며, 프로그램을 수행하는 기계의 환경과 메모리의 값들을 합친 것(composition)이다. 보통은 환경과 메모리 값의 조합이 유한하도록 설계한다.

 - _Program State = Environment X Memory (함수적 합성 : Cartesian Product)_

여기서 명령어들은 상태간의 전이(transition)라고 정의할 수 있다. 그리고 이러한 전이들을 순서대로 묶은 것을 루틴(routine)이라고 한다.

 - _Instruction : 명령, 프로그램 상태의 전이_
 - _Routine : 순서가 있는 명령 묶음_

그러면 이제 서브루틴과 코루틴으로 넘어갈 차례다.

### 관계

모든 프로그래머들은 코드에 대해서 자신만의 심상(Mental Model)을 갖기 마련이다. 서브루틴과 코루틴의 멘탈 모델 부터 시작해보고자 한다.

#### 서브루틴 : Caller-Callee

서브루틴의 관계는 일방적이다.
호출자(Caller)는 이미 어떤 상태에 있고(이전 조건), 어떤 루틴이 필요한지, 혹은 호출할지 알고있다.
피호출자(Callee)는 프로그래머의 의도대로 정의된 코드를 가지고 있고, 호출자와 무관하게 명령들을 수행한다. 여기서 부수효과(Side-effect)가 있을 수 있지만, 그건 명령의 내용에 대한 것이고 관계와는 무관하다.

![Fig: Relation of Subroutine](https://caiska.bn1304.livefilestore.com/y4mpAqgE-2ZtAMBzq_B4eBn8S35AG_sqM-mrzhBzQ-qZsc-8-UqnF-6q5QBBqI0mI_e-hWEhFwBVpu3kSSToMa0qIclnvogKeRTJA-i7eqm9ZvnF2TGgziVlsD8YRj9rZogyizLpBaxaHZ3P_RqwdE2AKu7ThSdJZsGKJk1LQnb3afhPDsBz6ClDhrMRpBS86eSxMD3fva5BM2VJiLfo9PE-g?width=660&height=425&cropmode=none)

#### 코루틴 : Activator-Activator
보통 코루틴은 cooperative한 routine이다. 이 때문에 협력루틴이라고 번역되기도 하는데, 이런 이름은 코루틴이 묵시적으로 다른 루틴과 함께 동작하도록 설계된 것을 의미한다.

![Fig: Relation of Coroutine](https://cairka.bn1304.livefilestore.com/y4mJI5CB46xuv-xi1oajDyVT-EwddQKm-1RINOP34GvInKfxXZMS_hMk2lITYLcPhDRSOA3e3LIDx4WU20SJ_0ns1k7cu8rV6j4XAtj690i9x2yj_UEJ85Hq2yw-ylxpztnHQ2eBOGnl3p0XFJ8O06Jn-Klc04LcZHiHqzZ7ONPO4mvSAmLDXvrwUmbAbSDwpXiqssEeSzAEftW7_qlK_VFfQ?width=828&height=374&cropmode=none )

그림에서 나온 루틴들은 서로를 활성화한다. 코루틴은 호출자(Invoker)와 재개루틴(Resuming routine)에 의해서 활성화 된다. 역으로 중단하거나 반환함으로써 코루틴은 다른 루틴을 재개시키게 된다. 그림에서는 Suspend/Return등으로 구분하였으나, 활성화라는 것은 실제로는 루틴 안의 임의의 지점(명령어)으로 점프(`goto`/`jmp`)하는 것을 의미한다. 따라서 중단하는 것이 다른 함수를 호출하는 것과 같을 수 있으며, 함수의 종료와 중단의 차이 또한 확연하지 않을 수 있다.

점프의 목표주소가 루틴의 도입부(함수라면 prologue)라면 활성화는 **Invocation** (`call`)이 된다.  
목표주소가 프로시저의 한가운데 위치하면, 이는 **Activation**(`resume`)이 된다.  

 - _Invocation: 호출. 루틴의 시작지점으로 점프하는 것_
 - _Activation: 활성화. 루틴의 임의 지점으로 점프하는 것_

서브루틴처럼, 코루틴은 이전 상태(Pre-condition)와 이후 상태(Post-condition)가 정의되어 있다. 이 말은, 코루틴 또한 프로그램 상태를 어떻게 전이시킬지 정의되어야 한다는 말이다.
하지만 서브루틴과는 다르게, 코루틴은 자신만의 상태를 유지한 채로 다른 주소로 `goto`할 수 있다. 이를 **Suspension**(`yield`)이라 한다. 경우에 따라 이는 프로시저의 종료, 혹은 함수의 반환(`return`)이 될수 있으며, 그때는 종료(finalization)를 함께 수행하게 된다.

 - _Suspension: 중단. 종료작업 없이 다른 지점으로 점프하는 것_
 - _Finalization: 종료. 루틴과 관련된 자원들을 정리(파괴)하고 다른 지점으로 점프하는 것_

#### 차이점?
요컨대 가장 큰 차이점은, 코루틴은 서브루틴보다 더 많은 동작(Operation)을 지원한다는 것이다. 이것이 도널드 크누스가 **The Art of Computer Programming**에서 이렇게 서술한 이유다.

> "Subroutines are special cases of ... coroutines"  - Donald Knuth

이를 표로 정리하면 다음과 같다.

| Operation     | Subroutine | Coroutine | |
|-------------- |:------:|:------:| --------|
| Invoke        | o      | o      | 프로시저를 시작한다
| Activate      | x      | o      | 특정한 지점으로 진입한다
| Suspend       | x      | o      | Control Flow를 멈춘다.
| Finalize      | o      | o      | 지역변수, 매개변수들을 정리하고 `return`한다.

서브루틴은 동작이 제한되었지만 관계에 대한 고려가 필요없어 간단하다. 반면에 코루틴은 더 많은 동작이 가능하지만, 중단과 활성화에 대한(그리고 각각의 Pre/Post condition에 대한) 정의가 필요하므로 더 복잡하다.

 - _Routine: 프로그램의 상태를 변화시키는 명령어 묶음. 그 내용은 프로그래머가 정의한다._
 - _Subroutine: 동작 2개를 지원하는 루틴_
    1. Invoke
    1. Finalize
 - _Coroutine: 동작 4개를 지원하는 루틴_
    1. Invoke
    1. Activate
    1. Suspend
    1. Finalize

#### 예제
크누스는 이런 개념이 어떻게 코드로 표현되는지 그의 책에서 설명하고 있다. 다음 코드는 The Art of Computer Programming에 있는 MIX 명령어들을 그대로 옮겨온 것으로, 모두 이해할 필요는 없다. 오늘날의 `call`/`return`구조와 달리, 모든 동작들이 `JMP` 명령어로 수행되는 것을 확인할 수 있다.

핵심은 **코루틴은 다수의 진입점과 중단점을 가진다**는 것이다. 더해서, 묵시적인 상태(레지스터 혹은 메모리)를 가질 수도 있다.

```
// MIX 예제, 주석을 변경하였음
// 자세한 내용은 The Art of Computer Programming 1.4.2 에 기술되어있다.

// Subroutine for character input
READER  EQU     16
INPUT   ORIG    *+16
NETCHAR STJ     9F
        JXNZ    3F          // ---> (3H)
1H      J6N     2F          // ---> (2H)
        IN      INPUT(READER)
        JBUS    *(READER)
        ENN6    16
2H      LDX     INPUT+16,6  // <--- (1H) J6N 2F
        INC6    1
3H      ENTA    0           // <--- JXNZ 3F
        SLAX    1
9H      JANZ    *
        JMP     NEXTCHAR+1

// First coroutine
2H      INCA    30          // <--- JGE 2B
        JMP     OUT         // ---> OUT1
IN1     JMP     NEXTCHAR    // ---> NETCHAR
                            //  <--
        DECA    30
        JAN     2B          // ---> (2H)
        CMPA    =10=
        JGE     2B          // ---> (2H)
        STA     *,1(0:2)
        ENT5    *
        JMP     NEXTCHAR    // ---> NETCHAR
                            //  <--
        JMP     OUT         // ---> OUT1
                            // <--- J5NN *-2
        DEC5    1
        J5NN    *-2         // ---> JMP OUT
        JMP     IN1         // ---> (IN1)

// Second coroutine
        ALF
OUTPUT  ORIG    *+16
PUNCH   EQU     17
OUT1    ENT4    -16         // <--- JMP  OUT
        MOVE    -1,1(16)
1H      JMP     IN          // ---> IN1
        STA     OUTPUT+16,4(1:1)
        CMPA    PERIOD
        JE      9F          // ---> (9H)
        JMP     IN          // ---> IN1
        STA     OUTPUT+16,4(2:2)
        CMPA    PERIOD
        JE      9F          // ---> (9H)
        JMP     IN          // ---> IN1
        STA     OUTPUT+16,4(2:2)
        CMPA    PERIOD
        JE      9F          // ---> (9H)
        INC4    1
        J4N     1B          // ---> (1H)
9H      OUT     OUTPUT(PUNCH)
        JBUS    *(PUNCH)
        JNE     OUT1        // ---> OUT1
        HLT
PERIOD  ALF     .
```

## 프로그래밍 모델

### 스택이다!

> "함정이다!" - 아크바 제독

오늘날 널리 사용되고 있는 High-level 언어들은 런타임 스택을 사용한다. 런타임 스택은 언어적으로 명확하게 정의되고, 자동으로 관리되며, 프로그래머가 개입하지 않아도 문제없이 동작한다.

구조적 프로그래밍 모델은 이런 스택중심의 모델과 조화를 이루어왔다. 이 모델에서 **프로그램은 수학적으로 표현할 수 있는 수식트리**라고 할 수 있다. 결과적으로 `main`함수는 종료하며, 적당한 결과값을 반환한다. 그림에서 표현한 것 처럼, 트리형 모델에서 함수의 흐름은 이 수식트리를 깊이-우선 순서로 순회하며, 작성하는 시점부터 각 구문(Statement)들의 실행 순서가 정해진다.

![Fig: Expression tree and Runtime stack](https://cwivka.bn1304.livefilestore.com/y4m3hNC7ri9XufcTvWshJc6x-FDX6cgTqyLZE_G08AEvbeK7I2nKexjWK5lvUbVArCiJgY0uB_pQwWhb2IpGXZHr2VJro2jjgThBiiDQi4_NC38QTittD7JuLRvhCBJTB2cMqd07w-9ELY9VAqDmIvbTW5F-TwjYf_RXyreZa5DHeNAFPeE_0FtLCLsjR1NAPMOJDJEM1yM0utxwtm4omOPTQ?width=1024&height=401&cropmode=none)

그림에서 B 구문에 도달하기 위해선, A 구문을 마쳐야 한다. 이 시점에 프로그램의 상태는 구문 A의 영향을 받아 변하게 되며, 레지스터/메모리는 적당한 값을 가지고 있을 것이다.

### Blocking

여기서 문제가 되는 것은 의존성이다.

트리 모델은 수학을 프로그램으로 옮겨놓은 것이다. 수식에 필요한 모든 값이 메모리에 있다면, 컴퓨터는 계산을 도중에 멈추지 않고도 수행할 수 있다. 다만 우리의 자원은 무한하지 않기 때문에, 프로그램 흐름이 시작되는 시점에 모든 값들을 준비할 수는 없다.

> 다른 원인도 있다. 값이 물리적으로 겁나 먼 곳에 있어서 실시간 통신을 해야할수도 있다.

결국 우리는 필연적으로 입출력을 해야한다. 전통적으로 입출력은 느리고, 결과를 기다리기 위해서 프로그램은 멈춰야 했다.

![Fig: Expression tree blocks](https://cwitka.bn1304.livefilestore.com/y4mU5R4khMBvmHhDnCCaAaX2FsvwsiDoJPtzrxGTXiYI8S9C-eKPFYSp8A7joRShjatPM9ymqZ4jQGnO7KHyO_hbbEjvO0sWFsPYtn_TcMFVF2DcS-mkjI-qedbz4ddNzEI0KShV9ZGm2DyBQF3uxkXrTH0iLk8nUMbcR_l0ZHD6vqCwkR_cJn3gRDmDPkziNNw6jmZ6QuEJhxKvVBDw9vzDw?width=1024&height=693&cropmode=none)

반응성(Responsiveness)을 위해 프로그램이 멈추기를 원하지 않는다면 어떨까? Non-blocking과 비동기(Asynchronous) 프로그래밍 모델은 여기서 시작한다. 이는 요약하자면 기존의 수식트리를 일반화 해서, 하나의 그래프를 작성하는 것이다. 상태기계가 그래프로 표현된다는 점을 생각해보라.

이 그래프 모델에선 **프로그램은 상태들로 이루어진 그래프**가 된다. 그리고 프로세서들은 이 그래프를 순회하는 것이다.

![Fig: Graph of states](https://cgivka.bn1304.livefilestore.com/y4mgGPMais5Nox_bo2a89cnv97CkD-aEUbF-vczXI3Z2ZCnwcFq4okkX1OZwAnymFxJqYun9_D7T1E8OTGI5CCsg49c3dnoUL6orCbrfGIhIcWBlw4Ni9mk3YHi4wvIjp4vePNyxVnNG56A7Y44GrVImH1qZgNBRt-kbNoPn3hOnikh2JWgpxkaBmpXjX3dWwah-dbQAOuy6otLqa62emMDOA?width=1024&height=681&cropmode=none )

> 프로세서는 운영체제의 프로세스, 스레드일 수 있다. 또는 언어레벨에서 지원하는 추상화, 가령 Erlang의 프로세스나 Go언어의 Goroutine일 수 있다. 더 이상의 자세한 설명은 생략한다.

### 영속성
핵심은, 그래프를 순회하면서 어떻게 정점간의 **문맥(context)**교환이 가능할까 하는 것이다. 우선 서브루틴은 `return`과 동시에 파괴되므로(사라지므로) 이것이 불가능하다.

달리 말해 서브루틴은 영속성이 없다. 이는 런타임 스택이 동작하는 방식이지만, 동시에 정점(프로그램 상태)간의 문맥전달을 아주 어렵게 만들었다. **콜백 지옥**도 결국은 문맥전달을 하기 위한 고육지책이라 할 수 있다.

> "제임스, 이 코드에는 버그가 있어요. 고칠 수 있겠어요?" - Gor Nishanov, CppCon2015

```c++
std::future<int64_t> tcp_reader(int64_t total)
{
    struct reader_state {
        std::array<char, 4096> _buffer;
        int64_t                _total;
        tcp::connection        _connection;

        explicit reader_state(int64_t total) :
            _total(total) {}
    };

    auto state = std::make_shared<reader_state>(total);

    return tcp::connect("127.0.0.1", 1337).then(
        [state](std::future<tcp::connection> the_connection) {
            state->_connection = std::move(the_connection.get());
            return do_while([state]() -> std::future<bool> {
                if (state->_total <= 0) {
                    return std::make_ready_future(false);
                }
                return state->conn.read(state->_buffer.data(),
                                        sizeof(state->_buffer)).then(
                    [state](std::future<int64_t> bytes_read_future) {
                        int64_t bytes_read = bytes_read_future.get();
                        if (bytes_read == 0) {
                            return std::make_ready_future(false);
                        }
                        state->_total -= bytes_read;
                        return std::make_ready_future(true);
                    });
           });
        });
}
```

## 접근법

우리에겐 영속성 있는 함수가 필요하다. 코루틴이 종료(finalization)하지 않으면서 중단(suspend/yield)이 가능했다는 점을 생각하면, 답은 명확하다. 코루틴이 있으면 이 문제를 해결할 수 있을 것이다. 하지만 어셈블리어로 구현하던 개념을 어떻게 스택 세상으로 가져올 수 있을까?

질문을 바꿔보자.

 1. 무엇이 영속성을 가져야 하는가?: **문맥**
 1. 문맥은 무엇인가?: **루틴의 상태**
 1. 상태는 무엇으로 이루어지는가?: **환경 & 메모리**
 1. 환경과 메모리는 어디에 위치하는가?: **함수 프레임(Function Frame)**

정리하자면, 질문은 **어떻게 함수 프레임들을 영속성있게 만들 것인가**라고 할 수 있다. 함수 프레임에 대한 설명은 자세히 기술하지는 않겠지만, 자세한 설명을 원한다면 [이 페이지](http://duartes.org/gustavo/blog/post/journey-to-the-stack/)를 참고하라.

### 스택 안? 밖!

> "왜 은행을 터나요?"
> "돈이 거기에 있잖아요"  - 어느 은행 강도

#### 스택 내(Stack-ful) 코루틴 / 분할 스택

언어마다 다르겠지만, C++는 함수 프레임을 스택에 배치한다. 이런 상황에서 프레임을 보존하려면, 프레임을 그대로 복사해서 어딘가에 저장해두는 수 밖에 없다. 다른 방법으로는 적당한 공간을 따로 예약해서 대체 스택(side stack)으로 사용하는 방법이 있을 것이다.

이 부분은 지나치도록 하겠다. MSVC는 스택 외 코루틴을 구현하고 있다.

#### 스택 외(Stack-less) 코루틴과 함수 프레임

N3858 제안서에는 이른바 '다시 시작하는 함수'(Resumable Function, 이하 재개함수)의 구현가능성에 대해 기술하고 있다. 그 방법론 중 하나는 힙(Heap)에 함수의 활성 프레임(Activation Frame)을 할당하는 것이다.

하지만 프레임은 단순하게는 구조체일 뿐이다. 코루틴의 동작 4개는 어떻게 된 것일까?
이들은 Compiler Intrinsic 형태로 MSVC가 자체적으로 구현하고 있다.

```c++
// Header File : <experimental/resumable>

// ...
// intrinsics used in implementation of coroutine_handle
extern "C" size_t _coro_resume(void *);
extern "C" void   _coro_destroy(void *);
extern "C" size_t _coro_done(void *);

#pragma intrinsic(_coro_resume)
#pragma intrinsic(_coro_destroy)
#pragma intrinsic(_coro_done)

// ...
// resumable functions support intrinsics
extern "C" size_t _coro_frame_size();
extern "C" void * _coro_frame_ptr();
extern "C" void   _coro_init_block();
extern "C" void * _coro_resume_addr();
extern "C" void   _coro_init_frame(void *);
extern "C" void   _coro_save(size_t);
extern "C" void   _coro_suspend(size_t);
extern "C" void   _coro_cancel();
extern "C" void   _coro_resume_block();

#pragma intrinsic(_coro_frame_size)
#pragma intrinsic(_coro_frame_ptr)
#pragma intrinsic(_coro_init_block)
#pragma intrinsic(_coro_resume_addr)
#pragma intrinsic(_coro_init_frame)
#pragma intrinsic(_coro_save)
#pragma intrinsic(_coro_suspend)
#pragma intrinsic(_coro_cancel)
#pragma intrinsic(_coro_resume_block)
// ...
```
그러니 이 부분에 대해서는 손댈 수 있는 부분이 없다. 더 자세한 설명이 필요하다면 참고자료에서 CppCon 발표들을 시청하기를 권한다.

 - Coroutine
    1. `_coro_resume`
    1. `_coro_destroy`
    1. `_coro_done`
    1. `_coro_save`
    1. `_coro_suspend`
    1. `_coro_cancel`
    1. `_coro_resume_addr`
 - Frame
    1. `_coro_frame_ptr`
    1. `_coro_frame_size`
    1. `_coro_init_frame`
 - Block
    1. `_coro_init_block`
    1. `_coro_resume_block`

> 이 intrinsic들 중 Frame과 Block들의 역할에 대해서 명확하지 않은 부분이 남아있는 것 같습니다. 기회가 되면 업데이트 하도록 하겠습니다.

## The C++ Coroutine : Resumable Function

이 시점에서 두 용어를 정리할 필요가 있을 것 같다.

 - _Coroutine : **개념**. 동작 4개를 지원하는 루틴_
 - _Resumable Function : C++ 언어에서 Coroutine의 **구현**방법_

추가로 다음 표는 코루틴의 동작들을 어떻게 사용하는지 정리한 것이다.

| Operation   | Subroutine    | Coroutine     |
|:------------|:-------------:|:-------------:|
| Invoke      |  `func(args)` | `func(args)`
| Activate    |  x            | `resume()`
| Suspend     |  x            | `co_yield`/`co_await`
| Finalize    |  `return`     | `co_return`

### 호출(Invoke)
**함수 호출에 대해서는 변경사항이 없다.** 하지만 재개함수는 `std::future<T>` 를 반환할 수 있다.

### 종료(Finalize)
서브루틴 내에서 반환지점은 익히 알려진 `return` 키워드를 사용하면 된다. 코루틴이라면 `co_return` 키워드가 이 역할을 대신한다.

### 중단(Suspend)
함수를 중단하고 Control Flow를 양보하는 방법은 `co_yield` expression과 `co_await` 연산자를 사용하는 것이다. 이 둘 중 하나라도 사용하면, 그 지점은 중단/재개 지점으로 사용되며 MSVC는 그 함수를 재개함수로 만들게 된다. (더해서 `co_return` 구문도 함수 내에 위치하면 재개함수로 인식된다.)

### 활성화(Activate)
중단된 함수를 활성화 할 때는, `coroutine_handle`과 멤버함수 `.resume()`를 사용한다. 이 호출은 Compiler Intrinsic인 `_coro_resume`위에 인터페이스를 올린 것이다.

## Code

> 예제 코드와 관련해 [kirkshoop의 저장소](https://github.com/kirkshoop/await)가 도움이 되었습니다.

미리 요약하자면, MSVC의 요구사항을 따르기만 한다면 C++에서도 코루틴을 사용할 수 있다. 제안서의 이름이 **재개**(따라서, 영속성 있는) 함수 라는 점에 주목하라. 재개함수에는 몇가지 종류가 있는데, 함수에 어떤 역할을 맡기고자 하느냐에 따라 달라진다. 그러면 컴파일러의 요구사항이란 무엇일까?

### Promise : 컴파일러가 원하는 것

C++11에 추가된 `std::future<T>`와 `std::promise<T>`, 그리고 이 두 짝궁의 관계에 대해서 알 것이라 전제하겠다. 일반적으로, `future<T>`는 "**나중에** 값이나 예외를 줄게"라는 것을 의미한다. (`std::async`를 참고하라) 그리고 값/예외는 `promise<T>`를 통해서 전달된다. 이는 "결과를 준다는 **약속**을 지켰다."라는 것이다.

앞서 재개함수가 `future<T>`를 반환한다고 설명했는데, 말 그대로다. 코루틴은 스스로 중단할 수 있기 때문에 활성화한 루틴 입장에서는 코루틴이 종료했는지 알 방법이 없다. 반환값이 있다는 것을 알 수 없는 것이다. 하지만 함수의 시그니처(Signature)상으로는, **나중에**라는 의미가 담긴 타입 `future`를 반환하도록 함으로써 일반적인 서브루틴처럼 사용할 수 있도록 하고 있다.

다만 기존과 달리 여기서의 `promise`는 재개함수를 위한것이다. [N4402](https://isocpp.org/files/papers/N4402.pdf)는 이 내용에 대해 기술하고 있다. 어째서 재개함수에게 약속이 필요한 것일까? 이는 재개함수가 **추상화**한 결과물이기 때문이다. 

스택 기반의 환경때문에, 코루틴 개념을 구현하기는 했지만 어셈블리와 같이 1차원으로 배치된(flat) 구문구조는 사용할 수 없었다. 재개함수는 결국 힙에서 살아있으며, 힙 영역(함수 프레임)에서 스택 영역(`future` 또는 awaitable)으로 값을 전달해야 하고, 이 과정은 일정한 **인터페이스**를 통해서 이루어지도록 한 것이다. 물론 인터페이스는 컴파일러와 프로그래머 양쪽 모두를 위한 것이다.

#### Resumable Promise Requirement
> 재개함수의 Promise 요구사항

그 인터페이스가 어떻게 쓰이는지는 잠시 후 기술하고, 그에 앞서 N4402 문서를 살펴보자. 재개함수에서 사용될 수 있는 Promise 타입은 아래 표에 열거된 멤버함수들을 지원해야 한다. 제안서를 요약한 것이므로, 보다 자세한 설명은 제안서를 참고하기 바란다.

| Expression              | Note |
|:------------------------|:-----
| `P{}`                   | 이 타입은 [default constructible](http://en.cppreference.com/w/cpp/concept/DefaultConstructible) 해야한다.
| `p.get_return_object()` | 재개함수의 반환값을 결정한다. `future<T>` 혹은 요구사항을 만족하는 사용자 정의 타입이 사용될 수 있다.
| `p.return_value(v)`       | `co_return` 구문. 결과값으로 `v`를 전달하며, 그 값은 나중에 사용(consume)된다.
| `p.return_value()`        | `co_return` 구문. `void` 타입에 사용되며, 결과값이 없음을 의미하기도 한다. 재개함수가 종료될때 사용된다. 이 함수가 호출되는 것은 "더이상 결과값이 없음"을 의미한다.
| `p.set_exception(e)`    | 예외를 전달한다. Resumer 측에서 결과값을 사용하려고 할때 `throw`된다.
| `p.yield_value(v)`      | `co_yield` 표현식. `return_value(v)`와 유사하다.
| `p.initial_suspend()`   | `true`를 반환할 경우, 초기 중단점(initial suspend point)에서 중단한다.
| `p.final_suspend()`     | `true`를 반환할 경우, 최종 중단점(final suspend point)에서 중단한다.

여기서 일부 함수들은 `coroutine_traits<T>`와 연관되어있는데, 이후에 이 타입의 유동성에 대해서 설명하겠다. 이제 C++ 코루틴의 사용방법으로 초점을 옮겨보자.


### Generator

#### 개념

 - _Generator = Iterator + Resumable Function_

반복자가 보이는가? `generator<T>`는 값의 수열(sequence)을 추상화 한 것이다. 이때 값은 재개함수에 의해서 생성되는데, 재개함수는 새로 호출되면 함수 프레임이 힙 영역에 유지되므로, `generator`마다 고유한 문맥을 가지게 된다.

#### Example
다음 코드는 N개의 피보나치 수를 생성하는 `generator`를 작성한 것이다.

```c++
#include <experimental/generator>

// N개의 피보나치 수를 생성하는 generator를 반환한다.
auto fibonacci(int n) noexcept
    -> std::experimental::generator<int>
{
    int f1 = 0;
    int f2 = 1;

    for (int i = 0; i < n; ++i) {
        // 피보나치 수를 지정(Set)하고 중단(Suspend)한다.
        co_yield f1;

        // 다음 피보나치 수를 구한 뒤 Shift 한다.
        int f3 = f1 + f2;
        f1 = f2;
        f2 = f3;
    }
    co_return;  // 더이상 값을 생성하지 않는다.
}

void usecase()
{
    // 10개의 피보나치 수를 받는다.
    for (int fibo : fibonacci(10))
    {
        // 0, 1, 1, 2, 3, 5, 8 ... 34
    }
}
```
어떻게 이게 가능한걸까? `generator<T>`의 정의를 살펴보자.

#### 실체(Detail)

간략하게 `generator<T>` 를 옮겨왔다. `iterator`와 `promise_type`을 지원하는 것을 확인할 수 있다. 또한 `begin()`/`end()` 함수로 [Range-based for loop](http://en.cppreference.com/w/cpp/language/range-for)를 지원하고 있다.

```c++
template <typename _Ty, typename _Alloc = allocator<char>>
struct generator
{
    // Resumable Promise Requirement
    struct promise_type;
private:
    // 재개함수를 위한 핸들
    coroutine_handle<promise_type> _Coro = nullptr;
public:
    struct promise_type { /*...*/ };
    struct iterator { /*...*/ };

    iterator begin()
    {
        // 함수 프레임이 있다면 재개해서 값을 받아올 수 있다.
        if (_Coro) {
            _Coro.resume();
            // 함수가 반환되었나?
            if (_Coro.done())
                return {nullptr}; // return end();
        }
        // 아직 끝나지 않았다.
        // 다음에 다시 resume하자...
        return {_Coro};
    }

    iterator end()
    {
        // 함수 프레임이 없다면, 값을 더 가져올 수 없다.
        return {nullptr};
    }

    ~generator()
    {
        // RAII : 함수 프레임을 파괴한다.
        if (_Coro) {
            _Coro.destroy();
        }
    }

};
```

이를 바탕으로 실제로는 이런 코드가 만들어질 것이다.

```c++
void usecase()
{
    // N개의 피보나치 수를 생성하는 generator를 반환한다.
    generator<int> gen = fibonacci(10);

    // `iter++`는 delete되었으므로 사용이 불가능하다.
    for (auto iter = gen.begin();
         iter != gen.end();
         ++iter )
    {
        int fibo = *iter;
        // 0, 1, 1, 2, 3, 5, 8 ... 34
    }
}
```
일반적인 `iterator-for` 구문과 크게 다르지 않다. `iterator`는 어떻게 되어있을까?

```c++
template <typename _Ty, typename _Alloc = allocator<char>>
struct generator
{
    // 반복자 인터페이스
    struct iterator : _STD iterator<input_iterator_tag, _Ty>
    {
        // Resumable function handle
        coroutine_handle<promise_type> _Coro;

        iterator(nullptr_t);
        iterator(coroutine_handle<promise_type> _CoroArg);

        // 다음으로 이동할때...
        iterator &operator++()
        {
            // 함수를 재개해서 값을 yield하도록 한다.
            _Coro.resume();

            // 만약 함수가 return했다면, 그 핸들은 generator가 소멸시킬 것이다.
            if (_Coro.done())
                _Coro = nullptr;
            return *this;
        }

        // MSVC의 코멘트는, 요약하자면,
        // 후위증가식은 iterator를 덮어쓸 수 있고,
        // 이로인해 handle이 nullptr로 바뀌면서
        // 함수 프레임 내에 마지막 값을 받아올 수 없다는 것이다.
        // ---- ---- ---- ---- ----
        // generator iterator current_value
        // is a reference to a temporary on the coroutine frame
        // implementing post increment will require storing a copy
        // of the value in the iterator.
        iterator operator++(int) = delete;
        //{
        //      auto _Result = *this;
        //      ++(*this);
        //      return _Result;
        //}

        // `promise`에 포인터가 있다. 포인터를 참조해 프레임 안의 값을 가져온다.
        _Ty const &operator*() const
        {
            return *_Coro.promise()._CurrentValue;
        }

        _Ty const *operator->() const;

        bool operator==(iterator const &_Right) const;
        bool operator!=(iterator const &_Right) const;

    };
};
```

이 코드를 보면 반복자가 함수를 반복적으로 재개하는 것을 알 수 있다. 그리고 `generator<T>`의  `promise_type`을 통해서 값을 획득한다. 이 경우는 `usecase` 함수가 **`generator`의 호출자면서 재개자인 것이다.**

![Fig: Generator Invoke and Resume](https://cgiska.bn1304.livefilestore.com/y4myyjBEM4mTFN8yloP0iVkJc-b8wQ_OzKXH-Y03gXYlJqaxpsg7db3xWQAxUujyjQamngJ9mw7cnHmFt3_W2qxviWYpMzdRR6iub1Msi_bZOHuB76FbOOdfrhDo5yen6OAzJH48zRRfbbiiboTzymHRLN5xOosm6PyywXhQO9lYo80KY_lPRhaUMu-dcPkbu3wiL0OtjtiyJE8iGt1-d4vbw?width=1024&height=607&cropmode=none)

#### 컴파일러의 시점
이제 핵심이 되는 `promise_type`을 볼 차례다. 기술된 것 보다 코드가 좀 더 있지만 단순화 하였다.

```c++
template <typename _Ty, typename _Alloc = allocator<char>>
struct generator
{
    // Resumable Promise Requirement for `generator`
    struct promise_type
    {
        // it knows where the value is... in frame.
        _Ty const *_CurrentValue;

        // It returns `promise_type`.
        // The reason will be explained in next section.
        promise_type &get_return_object()
        {
            return *this;
        }

        // If return `true`, suspends at initial suspend point.
        // So we suspends at *some* point
        bool initial_suspend()
        {
            return (true);
        }

        // If return `true`, suspends at final suspend point.
        // So we suspends at *some* point
        bool final_suspend()
        {
            return (true);
        }

        // Support `co_yield` expression
        void yield_value(_Ty const &_Value)
        {
            // point the value
            _CurrentValue = _STD addressof(_Value);
        }
        // ...
    }

    // ...
}
```
이 `promise_type`은 요구사항 중에서 4개의 멤버함수를 만족한다. `get_return_object`, `initial_suspend`, `final_suspend`, 그리고 `yield_value`가 정의되어 있다.
`co_yield`는 "값을 지정하고 중단하라"와 같은 의미인데, 컴파일러는 이를 이렇게 변경한다.

 - `co_yield` : `co_await` + `p.yield_value(x)`

따라서, `co_yield`에 인자 표현식이 없다는 것은 **불가능**하다. `x`는 `void`가 될 수 없기 때문이다. 같은 이유로 `generator<void>`는 컴파일 에러가 되며, 연속된 값의 추상화라는 정의에도 위배된다.

> 엄밀히 말해 여기서 `co_await`는 `suspend`를 의미한다. 자세한 설명은 후술한다.

```c++
#include <experimental/generator>

auto fibonacci(int n) noexcept
    -> std::experimental::generator<int>
{
    // ...
    for (int i = 0; i < n; ++i) {

        // 컴파일러는 이 표현식을 아래와 같이 바꾼다.
        // co_yield f1;
        p.yield_value(f1);  // 값에 대한 포인터를 지정하고
        co_await suspend_always{};              // 중단한다.

        // ...
    }
    co_return;  // 더이상 값이 없다.
}
```
이 코드에는 `promise_type p{}`같은 변수 선언이나, 다른 함수들의 호출이 없다. 그 이유는 다음 단원에서 다루겠다.


### Resumable Function
> 재개 함수

이번엔 좀 더 상향식으로 알아보자.  예제코드는 Awaitable Concept를 이해한 뒤에 봐야 한다.

#### 개념

 - _Resumable Function : Heap Allocated Frame + Operation_
 - _Operation : `call` | `co_await` | `co_return` | `resume()`_

`generator<T>`는 단순한 예제였다. `generator`는 `coroutine_handle`을 멤버변수로 가지고 있었는데, 이 타입은 무엇인지 살펴봐야 한다. 사실, `coroutine_handle`의 예전 이름은 `resumable_handle`였다.

재개함수는 C++상에서 코루틴 개념을 구현한 것이라고 설명했는데, `coroutine_handle`는 달리 말하면 재개함수에 대한 핸들(Resumable Function Handle)이다. Windows API를 사용해본 경험이 있다면 *Handle*이 결국 `void*`라는 것을 알 것이다. 여기서도 동일하다.

앞서 설명했던 것 처럼 MSVC는 힙 영역에 프레임을 동적할당하고, 거기에 약간의 intrinsic을 더하는 형태로 스택 외(stack-less) 코루틴을 구현하고 있다. **`coroutine_handle`은 이들을 하나의 타입으로 묶어놓은 것이다.**

#### 재개함수의 프레임

그러면, Resumable Frame 은 대체 어떻게 생겼을까? 일단 프레임은 Head와 Body로 나뉜다.

```c++
// TEMPLATE CLASS coroutine_handle
template <typename _PromiseT = void>
struct coroutine_handle;

// TEMPLATE CLASS coroutine_handle<void> - no promise access
template <>
struct coroutine_handle<void>
{
    // ---- ---- ---- ---- ----
    // 함수 프레임 Head 
    //  - Fn    : 재개할 때 수행할 명령어의 주소
    //  - cdecl : 이 Convention에서 함수 프레임의 정리는 Caller 가 담당한다.
    //  - Index : Resumption point의 인덱스
    //            0번은 함수가 종료된(`done()`) 경우에 사용한다.
    //  - Flag  : ???
    struct _Resumable_frame_prefix
    {
        typedef void(__cdecl *_Resume_fn)(void *);
        _Resume_fn _Fn;
        uint16_t _Index;
        uint16_t _Flags;
    };
    // ---- ---- ---- ---- ----
    // 함수 프레임 Body 에는...
    //  - Promise 타입 개체
    //  - 프레임으로 이동시킨 Arguments
    //  - 함수 Body
    //      - 지역/임시 변수들
    //  - Platform 전용
    //      - 레지스터 등등
    // ---- ---- ---- ---- ----

protected:
    _Resumable_frame_prefix *_Ptr;

public:
    coroutine_handle() noexcept = default;
    coroutine_handle(std::nullptr_t) noexcept;
    coroutine_handle &operator=(nullptr_t) noexcept;

    // Import : the pointer to resumable handle
    static coroutine_handle from_address(void *_Addr) noexcept
    {
        coroutine_handle _Result;
        _Result._Ptr = reinterpret_cast<_Resumable_frame_prefix *>(_Addr);
        return _Result;
    }

    // Export : return the frame address
    void *address() const noexcept
    {
        return _Ptr;
    }

    explicit operator bool() const noexcept;
    void resume() const;
    void destroy();

    bool done() const
    {
        // REVISIT: should return _coro_done() == 0; when intrinsic is
        // hooked up
        return (_Ptr->_Index == 0);
    }

    // ...
};
```

**Head** : 이름처럼 `_Resumable_frame_prefix`가 함수프레임의 앞부분이 된다. MSVC 는 이 공간에 `sizeof(void *) * 2`(x64 에서 16 바이트)만큼의 고정된 크기를 사용한다.

  - `Fn` :  `cdecl` 호출 규약으로 선언되어있다는 점이 특이한데, `cdecl`에서 프레임의 정리는 호출자의 역할이며, 따라서 `Fn`을 호출하는 것은 그 시점의 스택과 함수 프레임에 영향을 미치지 않는다.
  - `Index` : 재개함수는 다수의 중단/재개 지점들을 가지고 있고, `Index`는 각 지점들로 `switch`하는데 사용된다.  
  - `Flag` : ??? 이건 대체 뭘까 ???

**Body** : N4402에 함수 프레임의 구성에 대해 기술되어있다. James McNellis의 발표자료에 의하면, 컴파일러가 생성하는 프레임은 다음을 포함한다. 일반적인 스택 프레임과 크게 다르지 않다.

 - Promise
 - Captured arguments
 - Function body
       - Local Variables and Temporaries
 - Platform context
       - Registers

```c++
// `fibonacci` 함수를 위한 프레임(예시)
struct __frame_fibonacci
{
    // `_Resumable_frame_prefix`
    Frame_Prefix  _prefix;

    // Resumable Promise Requirement
    //   fibonacci는 `generator<int>`를 반환한다.
    generator<int>::promise_type  _p;

    // 프레임에 인자를 저장한다.
    int   _n; // fibonacci(n);

    int   _f1, _f2;     // 지역 변수
    int   _i,  _f3;     // 임시 변수

    // Platform 전용 저장소
    //  레지스터나 이것저것...
};
```

#### Coroutine Traits

`p.get_return_object()`에 대해서 다루지 않았었는데, 여기서 같이 다루고자 한다. 먼저 `coroutine_traits`를 눈여겨봐야 한다.

```c++
// <experimental/resumable>

// TEMPLATE CLASS coroutine_traits
template <typename _Ret, typename... _Ts>
struct coroutine_traits
{
    using promise_type = typename _Ret::promise_type;
};
```
굉장히 단순한데, `coroutine_traits`이 요구하는 것은 결국 `promise_type`를 구현하라는 것이다. 이에 대한 검사는 컴파일 시간에 이루어지고, 이 타입은 **Resumable Promise Requirement**를 만족시켜야 한다.

앞서 재개함수는 `future<T>`를 반환할 수 있다고 설명했는데, 사실 그것은 부정확한 설명이었다. `coroutine_traits`를 만족시키기만 한다면, 그 타입은 재개함수의 반환 타입이 될 수 있다.

`std::future<T>`를 위한 `coroutine_traits`의 **템플릿 특수화**버전을 옮겨왔다. `generator<T>`의 경우는 내부에 `promise_type`을 구현하고 있었는데, 예전부터 사용하던 타입들이나 커스텀 타입들은 이런 트릭을 사용해서 재개함수에서 유연하게 사용할 수 있다.

```c++
// <future> 헤더
#ifdef _RESUMABLE_FUNCTIONS_SUPPORTED

namespace std::experimental
{

// `std::future<T>`를 위한 템플릿 특수화
template<class _Ty, class... _ArgTypes>
struct coroutine_traits<future<_Ty>, _ArgTypes...>
{
    // `future<_Ty>`를 반환하는 재개함수들을 위한 타입정의
    struct promise_type
    {
        // `std::promise<T>`를 그대로 사용한다.
        promise<_Ty> _MyPromise;

        // 재개함수의 반환 타입은 `std::future<T>`...
        future<_Ty> get_return_object()
        {
            return (_MyPromise.get_future());
        }

        // `false`일 경우, 초기 중단점에서 멈춘다.
        bool initial_suspend() const
        {
            return (false);
        }

        // `false`일 경우, 최종 중단점에서 멈춘다.
        bool final_suspend() const
        {
            return (false);
        }

        // `std::promise<T>`를 통해 값을 전달한다.
        template<class _Ut>
        void return_value(_Ut&& _Value)
        {
            _MyPromise.set_value(_STD forward<_Ut>(_Value));
        }

        // `std::promise<T>`를 통해 예외를 전달한다.
        void set_exception(exception_ptr _Exc)
        {
            _MyPromise.set_exception(_STD move(_Exc));
        }
    };// struct promise_type
};// coroutine_traits<T, Args...>

// ...
}// namespace std::experimental

#endif
```

MSVC는  `co_await`/`co_yield`/`co_return` 키워드 중 하나를 확인하면, `coroutine_traits`를 사용해 컴파일 시간에 검사를 수행할 수 있도록 코드르 생성한다. MSVC가 하는 일을 확인하기 위해 `fibonacci` 예제로 다시 가보자.

> 이 코드는 틀린 형태일 수 있습니다. 정확한 메커니즘을 확인하게 되면 갱신하도록 하겠습니다.

```c++
#include <experimental/generator>

using namespace std;
using namespace std::experimental;

// Make a generator for `N` fibonacci numbers
auto fibonacci(int n) noexcept
    -> std::experimental::generator<int>
{
    // 별칭 : 반환타입 
    using return_type = generator<int>;
    // 반환타입이 Promise Requirement를 만족하는가?
    using traits = coroutine_traite<return_type>;

    // 요구사항을 만족한다면 이 재개함수를 위한 프레임을 생성한다.
    // fibonacci 함수를 위한 가상의 프레임. 위와 동일하다.
    struct __frame
    {
        // `_Resumable_frame_prefix`
        Frame_Prefix  _prefix;

        // Resumable Promise Requirement
        //   fibonacci returns `generator<int>`
        generator<int>::promise_type  _promise;

        // Captured arguments
        int   _n; // fibonacci(n);

        int   _f1, _f2;     // Local variable
        int   _i,  _f3;     // Temporaries

        // Platform dependent storage
        //  for registers, etc.
    };

    // 프레임을 동적으로 할당한다. context(ctx)라고 하겠다.
    // 특정한 Allocator를 사용해 커스터마이즈 할 수도 있지만 여기선 단순히 new로 설명한다.
    __frame* ctx = new __frame{std::move(n)};
    // 반환 개체를 생성한다.
    return_type __return = ctx->_promise.get_return_object();

    // true일 경우 중단, false일 경우 진행한다.
    if( ctx->_promise.initial_suspend() ){ // 항상 true
        // 중단한다...
    __initial_suspend_point:
    }

    // 사용자 코드는 프레임을 사용하도록 변경된다.
    // ---- ---- ---- ---- ----
    {
        ctx->_f1 = 0;
        ctx->_f2 = 1;

        for (ctx->_i = 0;
             ctx->_i < ctx->_n;
             ctx->_i++)
        {
            // promise를 통해 값을 전달한다.
            // co_yield f1;
            ctx->_promise.yield_value(ctx->_f1);

            // `ctx->_prefix`의 Index를 조정하고 중단한다.
            // co_await suspend_always{};
        __suspend_resume_point_1:

            // 다음 피보나치 수를 계산하고 Shift
            ctx->_f3 = ctx->_f1 + ctx->_f2;
            ctx->_f1 = ctx->_f2;
            ctx->_f2 = ctx->_f3;
        }

        // co_return;
        ctx->_promise.set_result();
        goto __final_suspend_point;
    }
    // ---- ---- ---- ---- ----

    if( ctx->_promise.final_suspend() ){ // 항상 true
        // 중단한다...
    __final_suspend_point:
    }

    // 정리작업...
}
```
생각보다 컴파일러가 많은 작업을, 하지만 기존에 스택 프레임에 대해서 하던것과 유사한 작업을 하는 것을 알 수 있었다. `ctx`에 대한 포인터를 사용해 힙영역에 위치한 변수를 사용하는 방법은 `this` 포인터의 사용과 다르지 않기 때문에, 이런 프레임 기반 구현의 비용은 추가적인 `co_await`관련 비용이외에는 무시할 수 있는 수준일 것이다.

 - `co_yield`  : `co_await` + `p.yield_value(x)`
 - `co_return` : `p.set_result(x)` + `goto final_suspend;`

#### 잠깐 정리
`co_return`과 `co_yield`에 대해서 `generator<T>` 예제를 통해 살펴봤다. Generator는 반복자와 재개함수를 합친 것이었다.

재개함수는 `coroutine_handle`을 사용하는데, 이 타입은 함수 프레임에 대한 포인터와 컴파일러 Intrinsic을 합쳐놓은 것이다. 프레임 기반의 구현은 `this`포인터를 사용해 변수를 조작하는 것과 유사하다.

컴파일 시간(정적) 검사를 위해 `coroutine_traits<T>`가 사용된다. 이를 통해 `promise_type`을 구현하도록 강제하며, 이 타입은 Resumable Promise Requirement를 만족해야 한다. `std::future<T>`의 경우 템플릿 특수화가 기본적으로 지원된다.


### "기다릴 수 있다"는 개념에 대해서

 - _`operator co_await` = Syntactic Sugar + Resumable Function_

재개함수의 마지막 핵심은 단항 연산자 `co_await`라고 할 수 있다. 
문맥상으로, **await는 suspend와 같은의미다**. 그리고 이 중단 때문에, 재개함수의 반환값은 자연스럽게 **비동기**가 된다.

멘탈 모델 상에서 이런 특성은 꽤 중요한 점인데, C# 언어의 `async` & `await`기능과 유사하다고 볼 수 있다. C++ 세상에서 다른점은, 프로그래머의 코드와 컴파일러의 지원을 사용해서 **수동으로** 관리할 수 있다는 점이다.

#### Example
MSVC 블로그의 예제를 가져왔다.

```c++
#include <iostream>
#include <chrono>
#include <future>
#include <windows.h>    // Windows Threadpool API

using namespace std;
using namespace std::literals;

// 연산자 오버로드
// co_await 는 기본 타입(primitive type) 인자는 받지 않는다.
auto operator co_await(chrono::system_clock::duration duration)
{
    using namespace std::experimental;

    // 기다리기 위해선 함수 3개를 구현해야 한다.
    //  - bool await_ready();
    //  - auto await_suspend();
    //  - T    await_resume();
    class awaiter
    {
        static
        void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE,
                                    void *Context,
                                    PTP_TIMER)
        {
            // Callback을 수행하는 Thread에서 Resume한다.
            coroutine_handle<>::from_address(Context).resume();
        }
        PTP_TIMER timer = nullptr;
        chrono::system_clock::duration duration;
    public:

        explicit
        awaiter(chrono::system_clock::duration d) : duration(d)
        {}
        ~awaiter() {
            if (timer) CloseThreadpoolTimer(timer);
        }

        // 준비가 안되었으면 (`false`), `await_suspend`를 호출한다.
        // 값이 준비되었으면 (`true`), `await_resume`를 바로 호출한다.
        bool await_ready() const
        {
            return duration.count() <= 0;
        }

        // 이 함수의 반환값은 무시될 수 있다.
        bool await_suspend(coroutine_handle<> resume_cb)
        {
            int64_t relative_count = -duration.count();
            timer = CreateThreadpoolTimer(TimerCallback,
                                          resume_cb.address(),
                                          nullptr);
            // 타이머를 세팅하고 중단한다....
            SetThreadpoolTimer(timer, (PFILETIME)&relative_count, 0, 0);
            return timer != 0;
        }

        // 재개된 후에는 T 타입 값을 반환한다.
        // `void`도 가능하다.
        void await_resume() {}

    };
    return awaiter{ duration };
}

// 코루틴(재개함수)
future<void> test()
{
    cout << this_thread::get_id() << ": sleeping…\n";

    // 1 밀리초를 기다린다...
    co_await 1ms;

    cout << this_thread::get_id() << ": woke up\n";
}

// 평범한 서브루틴
void usecase()
{
    test().get();
    cout << this_thread::get_id() << ": back in main\n";
}
```

이 코드는 대략 이런 식으로 진행된다.

![Fig: Invoker and Resumer for co_await](https://bqivka.bn1304.livefilestore.com/y4mfcDYFCH6SMe0hrR74BNJzFaua5hK_tksI8vp8pdRykhjrX1yuUOQoHrhFVFu9ci5icB6lOrMGLwaC74PxZthzgw8u-O4yNDAgEnkT5EMTPv7CcAVmOX6DHF_Ofi44GE33IJgYheftmKUrKX46k9SubyycOFSpsjxd_2Vj5bKAwzML-6geswyforIr-YIeBH1_B82cMEKrJnsOf5kJDOfbQ?width=1024&height=692&cropmode=none)

#### 기다리기 위한 인터페이스

`co_await`는 동작하기 위해서 특정한 인터페이스를 요구한다. 위에서 본 것 처럼, Awaitable Interface는 3가지 함수를 구현해야만 한다.

 - `await_ready`
 - `await_suspend`
 - `await_resume`

이들은 멤버함수일 수도 있고, 일반 함수로도 가능하다. `future<T>`역시 이 인터페이스를 구현하고 있다. 달리말해, `future<T>`가 `co_await`의 인자가 될수도 있다는 의미다. 아래 코드처럼 비효율적일 수도 있겠지만, 사용자 정의 타입에 대해서 간단한 오버로드 만으로도 사용할 수 있다는 장점이 있다.

```c++
// <future>
namespace std
{

template<class _Ty>
bool await_ready(future<_Ty>& _Fut)
{
    return (_Fut._Is_ready());
}

template<class _Ty>
void await_suspend(future<_Ty>& _Fut,
                   experimental::coroutine_handle<> _ResumeCb)
{
    // future를 await하면 스레드를 만들어서 detach시킨다.
    // change to .then when future gets .then
    thread _WaitingThread( [&_Fut, _ResumeCb]{
            _Fut.wait();
            _ResumeCb();
        });
    _WaitingThread.detach();
}

template<class _Ty>
auto await_resume(future<_Ty>& _Fut)
{
    return (_Fut.get());
}

}// namespace std
```

컴파일러 쪽은 어떨까? 이 코드가 어떻게 바뀔까?

#### 컴파일러의 시점

 - _`operator co_await` = Syntactic Sugar + Resumable Function_

`co_await`는 표현을 바꾼다는 점에서 결국 syntatic sugar라고 할수 있다. 인자타입이 Awaitable Interface를 알맞게 구현하기만 한다면, 컴파일러는 코드를 바꿔준다. 좀전의 예제코드를 좀더 자세하게 살펴보겠다. 

```c++
// Before...
// ---- ---- ---- ---- ----
future<void> test()
{
    // ...
    co_await 1ms;
    // ...
}

// After...
// ---- ---- ---- ---- ----
// `co_await`키워드가 있으므로 재개함수가 된다.
future<void> test()
{
    // 컴파일러는 이 함수를 위한 프레임을 생성한다...
    __frame* context;
    // ...

    // Awaitable interface를 구현하였다.
    // 정확하게는 코루틴 프레임 안에 이 변수가 생성될 것이다.
    awaiter aw = operator co_await(1ms);

    // 값이 준비되어 있는가?
    if(aw.await_ready() == false) // await_ready(aw);
    {
        // 값이 없으므로 기다려야 한다. 
        // 함수 프레임을 `coroutine_handle` 타입으로 감싸고...
        coroutine_handle<> resume_cb =
                coroutine_handle<>::from_address(context);

        // suspend 함수를 호출한다.
        aw.await_suspend(resume_cb);

        // 중단 직전에 프레임에 Index를 변경하고......
    __suspend_resume_point_1:
        // 중단/재개 지점이 여기에 있으므로 ready에서 true를 반환하면
        // 함수는 멈추지 않고 진행한다. 
    }

    // await_resume이 반환타입이 있다면 value에 값을 저장한다.
    // auto value = aw.await_resume();
    aw.await_resume(); // await_resume(aw);

    // ...
}
```
이제 인터페이스의 각 함수들에 대해서 짚어볼 수 있겠다.

 - _`await_ready` : "지금 값이 있는가?"_

`await_ready`은 분기점이 된다. 값이 준비되었다면 `await_resume`에서 바로 그 값을 사용할 수 있겠지만, 그렇지 않다면 `if` 구문 안쪽으로 분기해 함수를 중단한다.
`await_ready`함수가 언제나 `true`를 반환한다면 `if`구문 안쪽은 Dead Code가 되고, 컴파일러의 최적화 과정에서 소멸될 것이다. 이 경우는 프레임까지 소멸되고 일반 서브루틴처럼 최적화될 수 도 있다. 

중단점이 있는 유효 범위로 진입하면, 함수 프레임을 `coroutine_handle<>` 로 감싸서 인자로 넘겨준다. `coroutine_handle<>::from_address`함수가 이 역할을 해준다.

 - _`await_suspend` : "곧 루틴이 멈춥니다. 해야할 일이라도?"_

`await_suspend`는 `coroutine_handle<>`을 인자로 받고, 중단하기 전에 해야할 내용을 코드로 끼워넣을 수 있게 된다.
```c++
// Windows Thread Pool Timer를 사용한다.
//  Timer Callback에 프레임의 주소를 넘겨주는 방식으로 코루틴을 마무리한다.
class awaiter
{

    bool await_suspend(coroutine_handle<> resume_cb)
    {
        int64_t relative_count = -duration.count();
        timer = CreateThreadpoolTimer(TimerCallback,
                                    // 포인터로 변환한다.
                                    resume_cb.address(),
                                    nullptr);
        SetThreadpoolTimer(timer, (PFILETIME)&relative_count, 0, 0);
        return timer != 0;
    }

    // Windows Thread Pool에 있는 스레드가 Callback을 호출하게 된다.
    //  자연스럽게 Resumer Thread가 된다.
    static
    void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE,
                                void *Context,
                                PTP_TIMER)
    {
        // 프레임 포인터를 핸들로 변환, 그리고 재개.
        coroutine_handle<>::from_address(Context).resume();
    }
}
```

 - _`await_resume` : "여기 결과값과 실행 흐름이요."_

함수를 재개하면 중단지점으로 점프하며, 중단점이 있는 유효범위를 벗어나 `await_resume`을 호출한다. 중단하던 시점에는 결과값이 없었기 때문에, 함수가 재개되었다는 것은 결과값이 준비되었다는 의미가 된다. 만약 void라면 `co_await`아래에 있는 코드를 실행해야 하도록 실행흐름을 넘겨준다는 의미일 것이다.

![Fig: Awaiting Windows Threapool Timer](https://cgiuka.bn1304.livefilestore.com/y4mpoQFynRK0frHDJppknT-r10zBKbxf8AgNbV_lVSDl8WHEcPl7hGHltVblWdKQSeStvaegIMlhqTGiSZ0AF57wx5XOGV_T_8asbwKQsxOzs2X473nkhVGpS-AkoZJJfG2_dCA46XtuWEw6IHIk7_OePD1bV_BF1WcAGLqyTVTFps5Bl9UetnzIBKVyxrPb_NC_s0qqvHNR-DLF6KqZRiQBg?width=645&height=660&cropmode=none)

#### `co_await` Tricks

Kenny Kerr 와 James Mcnellis 는 이런 메커니즘을 사용한 트릭들에 대해서 설명하는데, 시간적 여유가 있다면 꼭 보기를 권한다.

CppCon 2016 : [Kenny Kerr & James McNellis "Putting Coroutines to Work with the Windows Runtime"](https://channel9.msdn.com/events/CPP/CppCon-2016/CppCon-2016-Kenny-Kerr--James-McNellis-Putting-Coroutines-to-Work-with-the-Windows-Runtime)


### Async Generator

#### 개념

 - _Async Generator = Awaitable iterator + Resumable function_

Generator와 매우 유사하지만, 다른점은 **반복자가 Awaitable하다**는 것이다. 달리 말하면, Awaitable `iterator`에 대해서 `for co_await`구문을 사용할 수 있다. [Kirkshoop의 설명](https://github.com/kirkshoop/await#async_generatort---each-value-arrives-later )이 도움이 될 것이다.

#### Example
이 기능을 제대로 사용해보지 못해 제대로 된 예제를 적지 못했다. 하지만 지금까지의 예제와 컴파일러의 시점에 대한 설명을 읽었다면 이 코드가 이해될 것이라 생각한다.

```c++
// 무한한 정수 수열
async_generator<int> infinite()
{
    for (int i = 0;; ++i) {
        co_yield i;
    }
}

std::future<void> usecase()
{
    // for co_await 구문
    for co_await (int v : infinite())
    {
        // `v`로 무언가 한다...
    }
}
```

`iterator`가 Awaitable하다고 했는데, 이는 `usecase`가 다음과 같다는 의미다.

```c++
std::future<void> usecase()
{
    // 기다릴 수 있는 반복자 : awaitable(async) iterator
    async_generator<int> gen = infinite();

    for (auto iter = co_await gen.begin();  // `begin()` 에 
                                            // `co_await`가 적용된다.
         iter != gen.end();     // `end()` 는 nullptr와 같다.
         co_await ++iter) // `operator++()`에 `co_await`가 적용된다.
    {
        int v = *iter;
        // ...
    }
}
```

`generator<T>` 예제와 다른 점은 함수가 2개 지점에서 반복자를 기다리고 있다는 것이다. 따라서 `begin()` 과 `operator++()`는 `usecase` 코루틴의 중단/재개지점이 된다.

여기선 `value_type`은 기본 자료형 `int`라서 어려운 코드는 없다. Awaitable iterator 코드는 다음처럼 만들 수 있다. 사실 `std::experimental::generator<T>`의 코드를 거의 그대로 가져온 것이다.

```c++
// generator가 아니라 iterator가 비동기로 동작한다.(awaitable).
template <typename T>
struct async_generator
{
    struct promise_type;  

    // Awaitable interface를 구현해야 한다.
    struct iterator :
            std::iterator<input_iterator_tag, T>
    {
        coroutine_handle<promise_type> chp = nullptr;

        // ...
        iterator operator++(int) = delete;
        iterator &operator++();
        // ...
        T const *operator->() const;
        T const &operator*() const
        {
            // promise_type has constant pointer to value
            const promise_type& prom = chp.promise();
            return *prom.pvalue;
        }
        // ...

        // 지금은 예제이므로, 언제나 값이 존재한다. 따라서 중단하지도 않는다.
        bool await_ready() const
        {
            return true;
        }

        // 중단하지 않으므로 내용이 없다.
        void await_suspend(coroutine_handle<> hcoro) {}

        // 이 함수의 반환값이 `co_await`에게 전달된다.
        iterator await_resume() const
        {
            return *this;
        }
    };// iterator

};// async_generator
```
멤버함수 3개를 `iterator`에 추가함으로써, awaitable 인터페이스를 구현하였고 `for co_await`구문을 쓸 수 있었다. 하지만 **Awaitable Interface** 단락에서 설명했듯 굳이 멤버함수일 필요는 없다.

```c++
using iter_type = async_generator<int>::iterator;

bool await_ready(iter_type)
{
    return true;
}

void await_suspend(iter_type & iter,
                   coroutine_handle<> hcoro)
{
    // do for suspension...
}

iter_type& await_resume(iter_type& it)
{
    return it;    // forward the reference
}
```
이렇게 보조 함수를 정의하는 것 만으로도 기존에 사용하던 `iterator` 타입들을 재활용할 수 있다.


## 끝! 후기
MSVC 코루틴의 개념과 코드를 훑어봤다. 그게 전부다. 부디 도움이 되었기를.

사실 이 Feature의 디스어셈블리 결과에 대해서도 쓰고 싶었는데, 관련해서는 아직 초보자 단계라 제대로 설명할 수가 없었다. 기회가 된다면 차후에 다시 글을 써보고 싶다.

나중에 더욱 개선할 기회가 있기를. :D


