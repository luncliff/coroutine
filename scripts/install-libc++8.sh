#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Build/Install libcxx & libcxxabi                                     "
echo "  - Version   : 8.0                                                   "
echo "  - Path      : /usr/{include, lib}                                   "
echo "  - Compiler  : gcc-8, g++-8                                          "
echo "                                                                      "
echo "----------------------------------------------------------------------"

# get the packages with specified version
echo "Download 8.0 ..."
if [ ! -f llvm.zip ]; then
    wget -q -O llvm.zip https://github.com/llvm-mirror/llvm/archive/release_80.zip &
fi
if [ ! -f libcxx.zip ]; then
    wget -q -O libcxx.zip https://github.com/llvm-mirror/libcxx/archive/release_80.zip &
fi
if [ ! -f libcxxabi.zip ]; then
    wget -q -O libcxxabi.zip https://github.com/llvm-mirror/libcxxabi/archive/release_80.zip &
fi
# wait for background downloads
for pid in $(jobs -p); do
    wait $pid
done
rm -rf wget*

# unzip and rename directories
echo "Unzip ..."
unzip -q llvm.zip &
unzip -q libcxx.zip &
unzip -q libcxxabi.zip &
# wait for background downloads
for pid in $(jobs -p); do
    wait $pid
done

mv ./llvm-release_80 ./llvm
mv ./libcxx-release_80 ./libcxx
mv ./libcxxabi-release_80 ./libcxxabi

echo "Build/Install ..."

export CC=gcc CXX=g++

# build libcxx
mkdir -p build-libcpp && cd ./build-libcpp
cmake ../libcxx \
    -DLLVM_PATH=../llvm \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS=../libcxxabi/include \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    ;
make -j7 install
rm CMakeCache.txt
cd -
# build libcxxabi
mkdir -p build-libcpp && cd ./build-libcpp
cmake ../libcxxabi \
    -DLLVM_PATH=../llvm \
    -DLIBCXXABI_LIBCXX_PATH=../libcxx/ \
    -DCMAKE_INSTALL_PREFIX=/usr \
    ;
make -j7 install
rm CMakeCache.txt
cd -
