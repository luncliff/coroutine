#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Install C++ Compilers                                                "
echo "  - Target Platform  : Ubuntu Xenial(16.04)                           "
echo "  - Path      : /usr/bin                                              "
echo "  - Compiler  : gcc-7, g++-7, gcc-8, g++-8, clang-7, clang-8          "
echo "  - Library   : libc++abi-7-dev                                       "
echo "                                                                      "
echo "----------------------------------------------------------------------"

apt update -qq
apt install -y -qq \
  build-essential software-properties-common
apt-add-repository -y ppa:ubuntu-toolchain-r/test

# http://apt.llvm.org/
# This library will use clang up to 'qualification branch'
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-7 main"
apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-8 main"

apt update -qq
apt install -y -q \
  ninja-build \
  g++-7 g++-8 clang-7 clang-8 libc++abi-7-dev

cd /usr/bin # pushd
ln -s --force gcc-8 gcc
ln -s --force g++-8 g++
ln -s --force clang-8 clang
cd - # popd
