#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Install C++ Compolers                                                "
echo "  - Target Platform  : Ubuntu Xenial(16.04)                           "
echo "  - Path      : /usr/bin                                              "
echo "  - Compiler  : gcc-7, g++-7, gcc-8, g++-8, clang-7                   "
echo "                                                                      "
echo "----------------------------------------------------------------------"

apt update -qq
apt install -y -qq \
  build-essential software-properties-common

apt-add-repository -y ppa:ubuntu-toolchain-r/test
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-7 main"

apt update -qq
apt install -y -q \
  ninja-build \
  g++-7 g++-8 clang-7 libc++-7-dev libc++abi-7-dev

cd /usr/bin # pushd
ln -s --force gcc-8 gcc
ln -s --force g++-8 g++
ln -s --force clang-7 clang
cd - # popd
