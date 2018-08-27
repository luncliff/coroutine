# Magic
Auto**magic**ally working C++ Coroutine: [Documentation](https://github.com/luncliff/Magic/wiki)

[![Build status](https://ci.appveyor.com/api/projects/status/9eoy07qfxxqghop3?svg=true)](https://ci.appveyor.com/project/luncliff/magic) [![Build Status](https://travis-ci.org/luncliff/Magic.svg)](https://travis-ci.org/luncliff/Magic)


[![SonarQube: Quality](https://sonarcloud.io/api/project_badges/measure?project=luncliff-magic&metric=alert_status)](https://sonarcloud.io/dashboard?id=luncliff-magic)
[![SonarQube: Coverage](https://sonarcloud.io/api/project_badges/measure?project=luncliff-magic&metric=coverage)](https://sonarcloud.io/dashboard?id=luncliff-magic)
[![SonarQube: Bugs](https://sonarcloud.io/api/project_badges/measure?project=luncliff-magic&metric=bugs)](https://sonarcloud.io/dashboard?id=luncliff-magic)
[![SonarQube: Code Smells](https://sonarcloud.io/api/project_badges/measure?project=luncliff-magic&metric=code_smells)](https://sonarcloud.io/dashboard?id=luncliff-magic)

## Build
For detailed build steps, reference [`.travis.yml`](/.travis.yml) and [`appveyor.yml`](/appveyor.yml).

#### Visual Studio 2017(vc141)
  - compiler option: [`/await`](https://blogs.msdn.microsoft.com/vcblog/2015/04/29/more-about-resumable-functions-in-c/) 
  - compiler option: `/std:c++latest`

#### Clang 6 for Windows 
Install following packages with [Chocolaty](https://chocolatey.org/). Uses CMake for project generation.
  - Chocolaty [LLVM package](https://chocolatey.org/packages/llvm)
  - Chocolaty [Ninja package](https://chocolatey.org/packages/ninja)

#### Clang for Linux
Since libc++ apt package doesn't contain experimental headers, the build steps [downloads libc++ and build with them](https://libcxx.llvm.org/docs/BuildingLibcxx.html).

#### AppleClang on MacOS
Trigger manual LLVM update before build. Required version is **stable 6.0.0** or later

## Package
Preparing Nuget for Visual Studio users. [The package](https://www.nuget.org/packages/CppMagic/).

## License 
**Feel free for any kind of usage**.

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
