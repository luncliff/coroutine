#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Install C++ Compilers                                                "
echo "  - Target Platform  : Ubuntu Bionic(18.04)                           "
echo "  - Path      : /usr/bin                                              "
echo "  - Compiler  : gcc-8, g++-8, gcc-9, g++-9, clang-7, clang-8, clang-9 "
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
apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-7 main"
apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main"
apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main"

apt update -qq
apt install -y -q \
  ninja-build \
  g++-8 g++-9 clang-7 clang-8 clang-9 libc++abi-7-dev

pushd /usr/bin
ln -s --force gcc-9 gcc
ln -s --force g++-9 g++
ln -s --force clang-8 clang
popd
