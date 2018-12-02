# coroutine

C++ Coroutine in Action

[![Build status](https://ci.appveyor.com/api/projects/status/vpjssf4g6cv4a4ys/branch/master?svg=true)](https://ci.appveyor.com/project/luncliff/coroutine/branch/master)
[![Build Status](https://travis-ci.org/luncliff/coroutine.svg?branch=master)](https://travis-ci.org/luncliff/coroutine)

## How To

### Build

Please reference [`.travis.yml`](./.travis.yml) and [`appveyor.yml`](./appveyor.yml) to see build steps

#### Tool Support

* Visual Studio 15 2017
* CMake + Clang 6.0+
  * `clang-cl`: for Windows with VC++ headers
  * `clang`: for Linux
  * `AppleClang`: for Mac

### Test

Test codes are in [test/](./test) directory

### Import

#### Visual Studio Project

For Visual Studio users, I recommend you to import [win32.vcxproj](./modules/win32.vcxproj) in [modules](./modules/).

#### CMake Project

Currently version doesn't export to for CMake's `find_package`.

```cmake
add_subdirectory(coroutine)

target_link_libraries(your_project
PUBLIC
    coroutine
)
```

## Features

See [`interface/`](./interface)

* channel  
    Similar to that of golang, but simplified form
* [`coroutine_handle`](./interface/coroutine/frame.h)  
    Helper code for compilers.
* [`class enumerable`](./interface/coroutine/enumerable.hpp)  
    Alternative type for `<experimental/generator>` where the header doesn't exists
* [`class sequence`](./interface/coroutine/sequence.hpp)  
    Asynchronous generator
