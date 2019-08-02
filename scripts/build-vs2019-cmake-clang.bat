
REM The batch script emulates the 'Visual Studio Native Command Prompt'
REM If the script fails, check the location of 'vcvarsall.bat' and invoke it

set CXX=clang-cl

REM Visual Studio Installer
REM     Enterprise/Professional/Community
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)
set vcvarsall_path="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist %vcvarsall_path% (
    call %vcvarsall_path% x86_amd64
)

cmake . -G "Visual Studio 16 2019" -DCMAKE_INSTALL_PREFIX="./install" -DBUILD_SHARED_LIBS="%SHARED%" -DCMAKE_BUILD_TYPE="%BUILD_TYPE%" -DCMAKE_CXX_COMPILER="clang-cl"
cmake --build .
cmake --build . --target install
