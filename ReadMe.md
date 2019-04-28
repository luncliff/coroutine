# coroutine

C++ Coroutine in Action.

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/38aa16f6d7e046898af3835918c0cd5e)](https://app.codacy.com/app/luncliff/coroutine?utm_source=github.com&utm_medium=referral&utm_content=luncliff/coroutine&utm_campaign=Badge_Grade_Dashboard)
[![Build Status](https://dev.azure.com/luncliff/personal/_apis/build/status/luncliff.coroutine?branchName=master)](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=13?branchName=master)
[![Build status](https://ci.appveyor.com/api/projects/status/vpjssf4g6cv4a4ys/branch/master?svg=true)](https://ci.appveyor.com/project/luncliff/coroutine/branch/master)
[![Build Status](https://travis-ci.org/luncliff/coroutine.svg?branch=master)](https://travis-ci.org/luncliff/coroutine)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=ncloc)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)

### Purpose of this library

* Help understanding of the C++ coroutine
* Provide meaningful design example with the feature

In that perspective, the library will be maintained as small as possible. Have fun with them. **And try your own coroutines !** 

### Signpost

* For coroutine example, visit the [test/](./test/) and then [interface/](./interface/coroutine)
* For the detail of C++ coroutine, visit the [wiki](https://github.com/luncliff/coroutine/wiki) or read the [`<coroutine/frame.h>`](./interface/coroutine/frame.h)

## Developer Note

### Architecture

**This library is for x64**.

### Tool Support

* [Visual Studio 2017 or later](./coroutine.sln)
  * `msvc` (vc141, vc142)
* [CMake](./CMakeLists.txt)
  * `msvc`
  * `clang-cl`: Works with VC++ headers. **Requires static linking**
  * `clang`: Linux
  * `AppleClang`: Mac

For clang users, I do recommend Clang 6.0 or later versions.

### [Interfaces](./interface)

To support multiple compilers, this library defines its own header, `<coroutine/frame.h>`. This might lead to conflict with existing library (libc++ and VC++).  
If there is a collision(build issue), please make an issue in this repo so I can fix it. 

```c++
// This header includes/overrides <experimental/coroutine>
#include <coroutine/frame.h>
```

Generator and async generator

```c++
#include <coroutine/yield.hpp>  // enumerable<T> & sequence<T>
```

Utility types are in the following headers

```c++
#include <coroutine/return.h> // return type examples for convenience
#include <coroutine/concrt.h> // concurrency utilities over system API
```

Go language style channel to deliver data between coroutines. Supports awaitable read/write and select operation are possible. But it is slightly differnt from that of Go language.

```c++
#include <coroutine/channel.hpp>  // channel<T, Lockable>
```

Network Asnyc I/O and some helper functions are placed in one header.

```c++
#include <coroutine/net.h>      // async i/o for sockets
```

## How To

### Build

Please reference the build configurations.  
Create an issue if you think another configuration is required.

* [Azure Pipelines](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=13?branchName=master)
  * Visual Studio 2017 (Visual Studio Solution File)
  * Visual Studio 2017 (CMake)
  * Ubuntu 16.04 + Clang 6.0
  * Mac OS + AppleClang
  * Windows + Clang-cl
* [`.travis.yml`](./.travis.yml)
  * Mac OS + AppleClang
  * Ubuntu 16.04 + Clang 7
  * iPhone OS : [leetal/ios-cmake](https://github.com/leetal/ios-cmake)
  * Android NDK (SDK 24 - 27) + Clang 8
* [`appveyor.yml`](./appveyor.yml)
  * Visual Studio 2017 (Visual Studio Solution File)
  * Windows + Clang-cl : [LLVM chocolatey package](https://chocolatey.org/packages/llvm)
* [Works on my machine :D](https://github.com/nikku/works-on-my-machine)
  * Visual Studio 2019
  * Windows Subsystem for Linux (Ubuntu 18.04 + Clang 7.1.0)
  * Clang-cl (LLVM 7.0.1, 8.0) + Ninja

### Test

Exploring [test(example) codes](./test) will be helpful. The library uses 2 tools for its test.

  * Visual Studio Native Testing Tool
  * CMake generated project with [Catch2](https://github.com/catchorg/catch2)

### Import

#### Visual Studio Project

For Visual Studio users,  
I recommend you to import(add reference) [windows.vcxproj](./modules/windows.vcxproj) in [modules](./modules/).

#### CMake Project

Expect there is a higher CMake project which uses this library. For Android NDK, the minimum version of CMake is **3.14**.

```cmake
cmake_minimum_required(VERSION 3.8) # Android NDK requires 3.14

# ...
add_subdirectory(coroutine)

# ...
target_link_libraries(your_project
PUBLIC
    coroutine
)
```

#### Package Manager

Following package managers and build options are available.

* [vcpkg](https://github.com/Microsoft/vcpkg/tree/master/ports/coroutine)
  * x64-windows
  * x64-linux
  * x64-osx

## License

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
