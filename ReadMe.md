# coroutine

C++ 20 Coroutines in Action

[![Build Status](https://dev.azure.com/luncliff/personal/_apis/build/status/luncliff.coroutine?branchName=master)](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=27&branchName=master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/38aa16f6d7e046898af3835918c0cd5e)](https://app.codacy.com/app/luncliff/coroutine?utm_source=github.com&utm_medium=referral&utm_content=luncliff/coroutine&utm_campaign=Badge_Grade_Dashboard)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)
[![](https://sonarcloud.io/api/project_badges/measure?project=luncliff_coroutine&metric=ncloc)](https://sonarcloud.io/dashboard?id=luncliff_coroutine)
![CMake](https://img.shields.io/badge/CMake-3.21+-00529b)

### Purpose of this repository

* Help understanding of the C++ Coroutines
* Provide meaningful design example with the feature

In that perspective, the library will be maintained as small as possible. Have fun with them. And try your own coroutines!
If you want some tool support, please let me know. I'm willing to learn about it.

**If you are looking for another materials, visit [the MattPD's collection](https://gist.github.com/MattPD/9b55db49537a90545a90447392ad3aeb#file-cpp-std-coroutines-draft-md)!**

* Start with the [GitHub Pages](https://luncliff.github.io/coroutine) :)  
  You will visit the [test/](./test/) and ~~[interface/](./interface/coroutine)~~ folder while reading the docs.
* This repository has some custom(and partial) implementation for the C++ Coroutines in the [`<coroutine/frame.h>`](./interface/coroutine/frame.h).  
  It can be activated with macro `USE_PORTABLE_COROUTINE_HANDLE`

### Toolchain Support

Currently using [CMake](./CMakeLists.txt) to generate buildsystem files with the following compilers.

* `msvc` v142+
* `clang-cl` 13+
* `clang` 12+
* `AppleClang` 12+
* `gcc` 10.0+

## How To

### Setup

```ps1
git clone "https://github.com/luncliff/coroutine"
Push-Location coroutine
  # ...
Pop-Location
```

### Test

Exploring [test(example) codes](./test) will be helpful. The library uses CTest for its test.
AppVeyor & Travis CI build log will show the execution of them.
