
REM The batch script emulates the 'Visual Studio Native Command Prompt'
REM If the script fails, check the location of 'vcvarsall.bat' and invoke it

REM VC++ Tools from Chocolatey package
REM     choco install  visualstudio2017-workload-vctools;
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)

REM Visual Studio Installer
REM     Enterprise/Professional/Community
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)

path "C:/Program Files/LLVM/bin/";%PATH%
clang-cl --version

REM We can specify with -DCMAKE_CXX_COMPILER=clang-cl
REM But for this script we will use CXX variable
set CXX=clang-cl

cmake . -G "Ninja" -DCMAKE_INSTALL_PREFIX="./install" -DBUILD_SHARED_LIBS="%SHARED%" -DCMAKE_BUILD_TYPE="%BUILD_TYPE%"
cmake --build .
cmake --build . --target install
