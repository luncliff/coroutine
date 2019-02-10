
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

set CXX=clang-cl
cmake ../ -G Ninja -DCMAKE_INSTALL_PREFIX=../install -DBUILD_SHARED_LIBS=false
ninja install
