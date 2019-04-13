# coroutine

C++ Coroutine in Action. [Wiki](https://github.com/luncliff/coroutine/wiki)

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/38aa16f6d7e046898af3835918c0cd5e)](https://app.codacy.com/app/luncliff/coroutine?utm_source=github.com&utm_medium=referral&utm_content=luncliff/coroutine&utm_campaign=Badge_Grade_Dashboard)
[![Build Status](https://dev.azure.com/luncliff/personal/_apis/build/status/luncliff.coroutine?branchName=master)](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=13?branchName=master)
[![Build status](https://ci.appveyor.com/api/projects/status/vpjssf4g6cv4a4ys/branch/master?svg=true)](https://ci.appveyor.com/project/luncliff/coroutine/branch/master)
[![Build Status](https://travis-ci.org/luncliff/coroutine.svg?branch=master)](https://travis-ci.org/luncliff/coroutine)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=luncliff_coroutine) [![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=ncloc)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)

The main goal of this library is ...

  * Help understanding of the C++ coroutine
  * Provide meaningful design example with the feature

In that perspective, the library will be maintained as small as possible. Have fun with them. **And try your own coroutines !** 

## Note

### [Interfaces](./interface)

To support multiple compilers, this library replaces `<experimental/coroutine>`. This might lead to conflict with existing library like libcxx and VC++.  
Prefer what you like. If the issue is severe, please create an issue.


```c++
// You can replace this header to <experimental/coroutine>
#include <coroutine/frame.h>
```

Generator and async generator

```c++
#include <coroutine/yield.hpp>  // enumerable<T> & sequence<T>
```

Utility types are in the following headers

```c++
#include <coroutine/return.h>   // return type for coroutine
#include <coroutine/suspend.h>  // helper type for suspend / await
#include <coroutine/concrt.h>   // synchronization utilities
```

Go language style channel to deliver data between coroutines

```c++
#include <coroutine/channel.hpp>  // channel<T, Lockable>
```

Network Asnyc I/O and some helper functions are placed in one header.

```c++
#include <coroutine/net.h>      // async i/o for sockets
```

### Build

Please reference [`.travis.yml`](./.travis.yml) and [`appveyor.yml`](./appveyor.yml) to see build steps.

#### Tool Support

  * Visual Studio 2017 or later
    * `msvc` (vc141)
  * CMake
    * `msvc`
    * `clang-cl`: Windows with VC++ headers. **Requires static linking**
    * `clang`: Linux
    * `AppleClang`: Mac

This library only supports x64

Expect Clang 6 or later versions. Notice that the feature, c++ coroutine, was available since Clang 5

### Test

Exploring [test(example) codes](./test) will be helpful.

  * Visual Studio Native Testing Tool
  * CMake generated project with [Catch2](https://github.com/catchorg/catch2)

### Import

#### Visual Studio Project

For Visual Studio users,  
I recommend you to import(add reference) [win32.vcxproj](./modules/win32.vcxproj) in [modules](./modules/).

#### CMake Project

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

#### Package Manager

Following package managers and build options are available.

* [vcpkg](https://github.com/Microsoft/vcpkg/tree/master/ports/coroutine)
  * x64-windows
  * x64-linux
  * x64-osx

## License

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
