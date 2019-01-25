# coroutine

C++ Coroutine in Action

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/38aa16f6d7e046898af3835918c0cd5e)](https://app.codacy.com/app/luncliff/coroutine?utm_source=github.com&utm_medium=referral&utm_content=luncliff/coroutine&utm_campaign=Badge_Grade_Dashboard)
[![Build Status](https://dev.azure.com/luncliff/personal/_apis/build/status/luncliff.coroutine?branchName=master)](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=13?branchName=master)
[![Build status](https://ci.appveyor.com/api/projects/status/vpjssf4g6cv4a4ys/branch/master?svg=true)](https://ci.appveyor.com/project/luncliff/coroutine/branch/master)
[![Build Status](https://travis-ci.org/luncliff/coroutine.svg?branch=master)](https://travis-ci.org/luncliff/coroutine)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=luncliff_coroutine) [![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=ncloc)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)

## How To

### Build

Please reference [`.travis.yml`](./.travis.yml) and [`appveyor.yml`](./appveyor.yml) to see build steps.

#### Tool Support

* Visual Studio 15 2017
  * `msvc`
* CMake
  * `clang-cl`: for Windows with VC++ headers
  * `clang`: for Linux
  * `AppleClang`: for Mac
  * `msvc`: Why not?

Expect Clang 6 or later versions. Notice that the feature, c++ coroutine, was available since Clang 5

### Test

Example/Test codes are in [test/](./test) directory.  
As mentioned above, the project supports 4 kind of tools.  
Visual Studio project uses its own testing tools. CMake generated project will create a test executable.

### Import

The project will NOT support package or prebuilt artifact until the feature comes into the standard.

#### Visual Studio Project

For Visual Studio users,  
I recommend you to import(add reference) [win32.vcxproj](./modules/win32.vcxproj) in [modules](./modules/).

#### CMake Project

Currently version doesn't export build results for CMake's `find_package`.
Expect there is a higher CMake project which uses this library.

```cmake
cmake_minimum_required(VERSION 3.8)

# ...
add_subdirectory(coroutine)

# ...
target_link_libraries(your_project
PUBLIC
    coroutine
)
```

## Features

Exploring [test(example) codes](./test) will be helpful.

#### [coroutine frame](./interface/coroutine/frame.h)

This library replaces `<experimental/coroutine>` to support multiple compilers.

```c++
namespace std::experimental
{
template <typename PromiseType = void>
class coroutine_handle;
}
```

#### [thread switching](./interface/coroutine/switch.h)

With this feature, [each routine can select thread](./test/thread/switch_to.cpp).

```c++
#include <coroutine/return.h>
#include <coroutine/switch.h>
#include <coroutine/sync.h>

using namespace std::literals;
const auto timeout = 2s;

void switch_to_background_thread()
{
    wait_group wg{};
    thread_id_t spawner, worker;

    auto continue_on = [&wg](thread_id_t target) noexcept(false)->unplug
    {
        spawner = current_thread_id();
        co_await switch_to{target}; // switch to designated thread
        // ...
        worker = current_thread_id(); // do some work in the thread
        // ...
        wg.done();  // notify work is finished
    };

    wg.add(1); // will wait for 1 coroutine
    continue_on(thread_id_t{0}); // start the coroutine
                                 // 0 means background thread
    REQUIRE(wg.wait(timeout) == true);
    // Before switching, it was on this thread.
    // After switching, the flow continued on another thread.
    REQUIRE(spawner == current_thread_id());
    REQUIRE(spawner != worker);
}
```

#### [channel](./interface/coroutine/channel.hpp)

Similar to that of [the Go language](https://golang.org/), but simplified form. [You can read before write](./test/vs/channel.cpp).

```c++
#include <coroutine/channel.hpp>
#include <coroutine/return.h>

// any lockable type will be fine
using channel_type = channel<uint64_t, std::mutex>;

auto write_to(channel_type& ch, uint64_t value) -> unplug
{
    bool closed = co_await ch.write(value))
}

auto read_from(channel_type& ch) -> unplug
{
    // tuple<uint64_t, bool>
    auto [value, closed] = co_await ch.read();
}
```

#### [generator](./interface/coroutine/enumerable.hpp)

Alternative type for `<experimental/generator>` where the header doesn't exists. [It is renamed to `enumberable<T>`](./test/vs/generator.cpp).

```c++
#include <coroutine/enumerable.hpp>
#include <coroutine/return.h>

TEST_METHOD(generator_for_accumulate)
{
    auto generate_values = [](uint16_t n) -> enumerable<uint16_t> {
        co_yield n;
        while (--n)
            co_yield n;

        co_return;
    };

    auto g = generate_values(10);
    auto total = std::accumulate(g.begin(), g.end(), 0u);
    Assert::IsTrue(total == 55);
}
```

#### [sequence](./interface/coroutine/sequence.hpp)

Asynchronous version of generator. That means, [both producer and consumer can suspend](./test/vs/async_generator.cpp).

```c++
#include <coroutine/sequence.hpp>
#include <coroutine/return.h>

TEST_METHOD(async_generator_yield_suspend_yield)
{
    suspend_hook sp{}; // helper to suspend / resume coroutine

    auto example = [&](int v = 0) -> sequence<int> {
        co_yield v = 444;
        co_yield sp;
        co_yield v = 555;
        co_return;
    };
    auto try_sequence = [&example](int& ref) -> unplug {
        for co_await(int v : example())
            ref = v;

        co_return;
    };

    int value = 0;
    try_sequence(value);
    Assert::IsTrue(value == 444);

    sp.resume();
    Assert::IsTrue(value == 555);
}
```

## License

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
