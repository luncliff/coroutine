# Magic
Auto**magic**ally working C++ Coroutine: [Documentation](https://github.com/luncliff/Magic/wiki)

[![Build status](https://ci.appveyor.com/api/projects/status/9eoy07qfxxqghop3?svg=true)](https://ci.appveyor.com/project/luncliff/magic) [![Build Status](https://travis-ci.org/luncliff/Magic.svg)](https://travis-ci.org/luncliff/Magic)

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

Build [Magic.sln](./Magic.sln) for Visual Studio environment. **[CMakeLists.txt](./CMakeLists.txt#L15) doesn't support this case.** Add reference to those project what you need.

#### Windows: Clang 6
Install following packages with [Chocolaty](https://chocolatey.org/). Uses CMake for project generation.
  - Chocolaty [LLVM package](https://chocolatey.org/packages/llvm)
  - Chocolaty [Ninja package](https://chocolatey.org/packages/ninja)

Clang + Windows uses [Ninja](https://ninja-build.org/) build system. [Follow this script](./appveyor.yml#L82).

#### Linux: Clang 5
The build step follows [Building libc++](https://libcxx.llvm.org/docs/BuildingLibcxx.html).

#### MacOS: AppleClang
Trigger manual LLVM update before build. Minimum version is **stable 6.0.0**. If you are using [`brew`](https://brew.sh/index), the following command will be enough.

```console
brew upgrade llvm;
```
### Test
> TBA

### Use
> TBA

## Package
Preparing Nuget package for Visual Studio users.

## License 
**Feel free for any kind of usage**.

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
