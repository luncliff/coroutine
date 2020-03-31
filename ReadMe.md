# coroutine

C++ 20 Coroutines in Action

[![Build Status](https://dev.azure.com/luncliff/personal/_apis/build/status/luncliff.coroutine?branchName=master)](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=27&branchName=master)
[![Build status](https://ci.appveyor.com/api/projects/status/vpjssf4g6cv4a4ys/branch/master?svg=true)](https://ci.appveyor.com/project/luncliff/coroutine/branch/master)
[![Build Status](https://travis-ci.org/luncliff/coroutine.svg?branch=master)](https://travis-ci.org/luncliff/coroutine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/38aa16f6d7e046898af3835918c0cd5e)](https://app.codacy.com/app/luncliff/coroutine?utm_source=github.com&utm_medium=referral&utm_content=luncliff/coroutine&utm_campaign=Badge_Grade_Dashboard)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=ncloc)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)

### Purpose of this repository

* Help understanding of the C++ Coroutines
* Provide meaningful design example with the feature

In that perspective, the library will be maintained as small as possible. Have fun with them. And try your own coroutines!

**If you are looking for another materials, visit [the MattPD's collection](https://gist.github.com/MattPD/9b55db49537a90545a90447392ad3aeb#file-cpp-std-coroutines-draft-md)!**

* Start with the [GitHub Pages](https://luncliff.github.io/coroutine) :)  
  You will visit the [test/](./test/) and [interface/](./interface/coroutine) folder while reading the docs.
* This repository has some custom(and partial) implementation for the C++ Coroutines in the [`<coroutine/frame.h>`](./interface/coroutine/frame.h).  
  It can be activated with macro `USE_PORTABLE_COROUTINE_HANDLE`

### Pre-requisite

* [Microsoft GSL v2.0.0 or later](https://github.com/microsoft/gsl)

The installation of this library will install it together.
All other required modules for build/test will be placed in [external/](./external).

### Tool Support

* Visual Studio 2017 or later
  * `msvc` (vc141, vc142)
* [CMake](./CMakeLists.txt)
  * `msvc`
  * `clang-cl`: Works with VC++ headers
  * `clang`: Linux
  * `AppleClang`: Mac

For Visual Studio users, please use **15.7.3 or later** versions.  
For clang users, I recommend **Clang 6.0** or later versions.

## How To

### Setup

Simply clone and initialize submodules recursively :)

```bash
git clone https://github.com/luncliff/coroutine;
pushd coroutine
  git submodule update --init --recursive;
popd
```

### Build

Please reference the build configurations.  
Create an issue if you think another configuration is required.

* [Azure Pipelines](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=13?branchName=master)
  * Visual Studio 2017 / 2019
  * Ubuntu 16.04 + Clang 8
  * Mac OS X + AppleClang 11
  * Windows + Clang-cl (LLVM 8.0.1+)
* [Travis CI](./.travis.yml)
  * Mac OS X + AppleClang 11
  * Ubuntu 16.04(Xenial)/18.04(Bionic) + Clang 8
  * Ubuntu 20.04 + GCC 10 ~~it failes !!!~~
* [AppVeyor](./appveyor.yml)
  * Visual Studio 2017 / 2019

### Test

Exploring [test(example) codes](./test) will be helpful. The library uses CTest for its test.
AppVeyor & Travis CI build log will show the execution of them.

### Import

If you want some tool support, please let me know. 
I'm willing to learn about it.

#### CMake 3.12+

Expect there is a higher CMake project which uses this library.

The library exports 3 targets.

* coroutine_portable
  * `<coroutine/frame.h>`
  * `<coroutine/return.h>`
  * `<coroutine/channel.hpp>`
* coroutine_system
  * requires: coroutine_portable
  * `<coroutine/windows.h>`
  * `<coroutine/linux.h>`
  * `<coroutine/unix.h>`
  * `<coroutine/pthread.h>`
* coroutine_net 
  * requires: coroutine_system
  * `<coroutine/net.h>`

```cmake
cmake_minimum_required(VERSION 3.12)

find_package(coroutine CONFIG REQUIRED)
# or add_subdirectory(coroutine) if you want to be simple

target_link_libraries(main
PUBLIC
    coroutine_portable
    coroutine_system
    coroutine_net
)
```

## Developer Note

### [Interface](./interface)

#### Portable

To support multiple compilers, this library defines its own header, `<coroutine/frame.h>`. This might lead to conflict with existing library (libc++ and VC++).  
If there is a collision(build issue), please make an issue in this repo so I can fix it. 

```c++
// This header includes/overrides <experimental/coroutine>
#include <coroutine/frame.h>
```

Utility types are in the following headers. With the macro `USE_EXPERIMENTAL_COROUTINE`, you can enforce `<experimental/coroutine>` instead of `<coroutine/frame.h>`

```c++
// return/promise types for coroutine functions
#define USE_EXPERIMENTAL_COROUTINE 
#include <coroutine/return.h> 
```

Generator is named `coro::enumerable` here.

For now you can see various description for the concept in C++ conference talks in Youtube.  
If you want better implementation or want to see another generators, visit the https://github.com/Quuxplusone/coro :D

```c++
// enumerable<T>
#include <coroutine/yield.hpp>
```

Go language style channel to deliver data between coroutines. 
It Supports awaitable read/write and select operation are possible.  
If you don't know the language, never worry. There was a talk in CppCon

* [CppCon 2016: John Bandela "Channels - An alternative to callbacks and futures"](https://www.youtube.com/watch?v=N3CkQu39j5I)

But it is slightly different from that of the Go language because we don't have a built-in scheduler in C++. Furthermore Goroutine is quite different from the C++ Coroutines.  
It may not a necessary feature since there are so much of the channel implementation, but you may feel curiosity about it.

```c++
// channel<T> with Lockable
#define USE_EXPERIMENTAL_COROUTINE 
#include <coroutine/channel.hpp>
```

#### System

The library doesn't provides platform-neutral abstraction.

```c++
// #include <gsl/gsl>             // requires ms-gsl
// #include <coroutine/return.h>  // already used by the following headers
#include <coroutine/windows.h>
#include <coroutine/linux.h>
#include <coroutine/unix.h>
#include <coroutine/pthread.h>
```

Please reference test codes for their usage.

#### Network

Async I/O with awaitable types for socket operation and `poll_net_tasks` to multiplex control flow. 

```c++
#include <coroutine/net.h>
```

Please reference test codes for its usage.

## License

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
