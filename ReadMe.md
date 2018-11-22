# coroutine

C++ Coroutine in Action

[![Build status](https://ci.appveyor.com/api/projects/status/vpjssf4g6cv4a4ys?svg=true)](https://ci.appveyor.com/project/luncliff/coroutine)
[![Build Status](https://travis-ci.org/luncliff/coroutine.svg?branch=master)](https://travis-ci.org/luncliff/coroutine)

## How To

### Build

Please reference [`.travis.yml`](./.travis.yml) and [`appveyor.yml`](./appveyor.yml)

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

For Visual Studio users, I recommend you to import vcxproj in [modules/win32](./modules/win32).

#### CMake Project

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
* `coroutine_handle`  
    Helper code for compilers. Releated issue: #1
* [`class enumerable`](./interface/coroutine/enumerable.hpp)  
    Alternative type for `<experimental/generator>` where the header doesn't exists
* [`class sequence`](./interface/coroutine/sequence.hpp)  
    Asynchronous generator
