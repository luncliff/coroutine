#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Build/Install libcxx & libcxxabi                                     "
echo "  - Version   : 7.0                                                   "
echo "  - Path      : /usr/{include, lib}                                   "
echo "  - Compiler  : gcc-7, g++-7                                          "
echo "                                                                      "
echo "  - Tools     :                                                       "
echo "      gcc-7, g++-7, libc++-dev libc++abi-dev                          "
echo "      wget, unzip, cmake                                              "
echo "                                                                      "
echo "----------------------------------------------------------------------"

# get the packages with specified version
echo "Download"
if [ ! -f llvm.zip ]; then
    wget -q -O llvm.zip         https://github.com/llvm-mirror/llvm/archive/release_70.zip &
fi
if [ ! -f libcxx.zip ]; then
    wget -q -O libcxx.zip       https://github.com/llvm-mirror/libcxx/archive/release_70.zip &
fi
if [ ! -f libcxxabi.zip ]; then
    wget -q -O libcxxabi.zip    https://github.com/llvm-mirror/libcxxabi/archive/release_70.zip &
fi
# wait for background downloads
for pid in `jobs -p`
do
    wait $pid
done
rm -rf wget*

# unzip and rename directories
echo "Unzip"
if [ ! -d llvm ]; then
    unzip -q llvm.zip &
fi
if [ ! -d libcxx ]; then
    unzip -q libcxx.zip &
fi
if [ ! -d libcxxabi ]; then
    unzip -q libcxxabi.zip &
fi
# wait for background downloads
for pid in `jobs -p`
do
    wait $pid
done

mv ./llvm-release_70        ./llvm;
mv ./libcxx-release_70      ./libcxx;
mv ./libcxxabi-release_70   ./libcxxabi;

echo "Build/Install with GCC"

export CC=gcc-7
export CXX=g++-7

# build libcxx
mkdir -p prebuilt && cd ./prebuilt;
    cmake ../libcxx                                         \
        -DLLVM_PATH=../llvm                                 \
        -DLIBCXX_CXX_ABI=libcxxabi                          \
        -DLIBCXX_CXX_ABI_INCLUDE_PATHS=../libcxxabi/include \
        -DCMAKE_BUILD_TYPE=Release                          \
        -DCMAKE_INSTALL_PREFIX=/usr                         \
        ;
    sudo make -j7 install;
    rm CMakeCache.txt;
cd -;
# build libcxxabi
mkdir -p prebuilt && cd ./prebuilt;
    cmake ../libcxxabi                      \
        -DLLVM_PATH=../llvm                 \
        -DLIBCXXABI_LIBCXX_PATH=../libcxx/  \
        -DCMAKE_INSTALL_PREFIX=/usr         \
        ;
    sudo make -j7 install;
    rm CMakeCache.txt;
cd -;
