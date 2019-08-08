#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Build/Install libcxx & libcxxabi                                     "
echo "  - Version   : 8.0                                                   "
echo "  - Path      : /usr/{include, lib}                                   "
echo "  - Compiler  : CC, CXX                                               "
echo "                                                                      "
echo "----------------------------------------------------------------------"

# get the packages with specified version
echo "Download Libcxx, libcxxabi, llvm 8.0 ..."
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
rm llvm.zip;
rm libcxx.zip;
rm libcxxabi.zip;
mv ./llvm-release_80        /tmp/llvm
mv ./libcxx-release_80      /tmp/libcxx
mv ./libcxxabi-release_80   /tmp/libcxxabi

echo "Build/Install ..."
# build libcxx
mkdir -p build-libcxx && cd ./build-libcxx
cmake /tmp/libcxx \
    -DLLVM_PATH=/tmp/llvm \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS=/tmp/libcxxabi/include \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr ;
make -j7 install
cd -

# build libcxxabi
mkdir -p build-libcxxabi && cd ./build-libcxxabi
cmake /tmp/libcxxabi \
    -DLLVM_PATH=/tmp/llvm \
    -DLIBCXXABI_LIBCXX_PATH=/tmp/libcxx/ \
    -DCMAKE_INSTALL_PREFIX=/usr ;
make -j7 install
cd -
