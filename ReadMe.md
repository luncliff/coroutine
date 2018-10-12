# Magic
Auto**magic**ally working C++ Coroutine: [Documentation](https://github.com/luncliff/Magic/wiki)

[![Build status](https://ci.appveyor.com/api/projects/status/ihhn5vx1jfp3hnqn?svg=true)](https://ci.appveyor.com/project/luncliff/magic) [![Build Status](https://travis-ci.org/luncliff/Magic.svg?branch=master)](https://travis-ci.org/luncliff/Magic) [![Build Status](https://dev.azure.com/luncliff/personal/_apis/build/status/luncliff.Magic)](https://dev.azure.com/luncliff/personal/_build/latest?definitionId=7)

[![SonarQube: Quality](https://sonarcloud.io/api/project_badges/measure?project=cppmagic&metric=alert_status)](https://sonarcloud.io/dashboard?id=cppmagic)
[![SonarQube: Coverage](https://sonarcloud.io/api/project_badges/measure?project=cppmagic&metric=coverage)](https://sonarcloud.io/dashboard?id=cppmagic)
[![SonarQube: Bugs](https://sonarcloud.io/api/project_badges/measure?project=cppmagic&metric=bugs)](https://sonarcloud.io/dashboard?id=cppmagic)
[![SonarQube: Code Smells](https://sonarcloud.io/api/project_badges/measure?project=cppmagic&metric=code_smells)](https://sonarcloud.io/dashboard?id=cppmagic)

## How To

### Build

For detailed build steps, reference [`.travis.yml`](/.travis.yml) and [`appveyor.yml`](/appveyor.yml).

#### Windows: Visual Studio 2017(vc141)

  - compiler option: [`/await`](https://blogs.msdn.microsoft.com/vcblog/2015/04/29/more-about-resumable-functions-in-c/) 
  - compiler option: `/std:c++latest`

Build [Magic.sln](./Magic.sln) for Visual Studio environment. **[CMakeLists.txt](./CMakeLists.txt#L15) doesn't support this case.** Add reference to the project that you need.

#### Windows: Clang 6

[Follow this script](./appveyor.yml#L82).
Clang + Windows uses [Ninja](https://ninja-build.org/) build system and CMake for project generation. 
It requires [Chocolaty](https://chocolatey.org/) and the following packages.

  - [LLVM](https://chocolatey.org/packages/llvm)
  - [Ninja](https://chocolatey.org/packages/ninja)

#### Linux: Clang 5

The build step follows [Building libc++](https://libcxx.llvm.org/docs/BuildingLibcxx.html). [`.travis.yml`](/.travis.yml)

#### MacOS: AppleClang

Trigger manual LLVM update before build. Minimum version is **stable 6.0.0**. 
If you are using [`brew`](https://brew.sh/index), the following command will be enough.

```console
brew upgrade llvm;
```

### Test

You can use [`cppmagic_vstest.dll`](/appveyor.yml) for Visual Studio Native Test. For the other case, you can use [`cppmagic_test`](/.travis.yml).
Android or iPhone OS doesn't support test for now.

```bash
cd path/to/cppmagic;  # Case of MacOS

mkdir -p build; pushd build;
  cmake ../ -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install;
  ninja;

  ./cppmagic_test; # After build, you can run the test program
popd;
```

### Use

For CMake based project, use `add_subdirectory`. But notice that it doesn't support windows.

```cmake
if(NOT TARGET cppmagic) # project name: `cppmagic`
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cppmagic)
endif()
```

## Package

> Not in plan for now

~~Nuget package for Visual Studio users.~~

## License 

**Feel free for any kind of usage**.

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
